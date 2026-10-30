#include "control.h"
#include "control_tuning.h"
#include "line_estimator.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=270000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p,unsigned base) {
    unsigned i; Control_Defaults(p); p->base_percent=p->straight_percent=base;
    Control_Start(s); s->speed=s->launch=base;
    for(i=0;i<70;++i) Control_Step(s,p,0,1,0x18,2);
}
static void Calibrate(LineEstimator *line,int high) {
    unsigned i,j; uint16_t raw[8];
    Line_Init(line); Line_BeginCalibration(line);
    for(j=0;j<60;++j) { for(i=0;i<8;++i) raw[i]=(j&1)?3800:200; Line_Update(line,raw); }
    CHECK(Line_EndCalibration(line));
    for(i=0;i<8;++i) raw[i]=high?200:3800;
    CHECK(Line_SetWhiteReference(line,raw));
}
static void Estimator(void) {
    LineEstimator line; uint16_t raw[8]; unsigned i,mask,high;
    for(high=0;high<2;++high) {
        Calibrate(&line,high);
        for(mask=0;mask<256;++mask) {
            unsigned groups=0,previous=0,left=0,right=0,total,diff; int expected;
            for(i=0;i<8;++i) {
                unsigned on=(mask>>i)&1;
                raw[i]=(on==(high!=0))?3800:200;
                if(on && !previous) ++groups;
                previous=on; if(i<4) left+=on*1000; else right+=on*1000;
            }
            total=left+right; diff=left>right?left-right:right-left;
            expected=mask && mask!=255 && (groups==1 || (groups==2 && diff>=300 && diff*4>=total));
            Line_Update(&line,raw);
            CHECK(line.mask==mask && line.valid==expected);
            CHECK(line.position_confidence==(expected?(groups==1?1:2):0));
            if(expected) CHECK(line.error>=-3500 && line.error<=3500);
        }
        /* 98 retains its real gap, but supplies a bounded directional observation. */
        for(i=0;i<8;++i) raw[i]=(((0x98>>i)&1)==high)?3800:200;
        Line_Update(&line,raw); CHECK(line.valid && line.position_confidence==2 && line.mask==0x98);
    }
    /* Preserve connected weak shoulders, reject distant weak islands and flat calibration. */
    Calibrate(&line,0);
    for(i=0;i<8;++i) raw[i]=3800;
    raw[3]=1280; raw[4]=2360; /* Strength700/400: only eye3 crosses mask threshold. */
    Line_Update(&line,raw);
    CHECK(line.valid && line.mask==8 && line.position_confidence==1 && line.error>-200 && line.error<0);
    raw[4]=3800; raw[7]=2720; /* Disconnected weak shoulder must not move the centroid. */
    Line_Update(&line,raw); CHECK(line.valid && line.error==-500);
    for(i=0;i<8;++i) raw[i]=(i&1)?200:3800;
    CHECK(Line_SetWhiteReference(&line,raw) && line.raw_high_mask==0xAA);
    for(mask=0;mask<256;++mask) {
        for(i=0;i<8;++i) raw[i]=(((mask>>i)&1)==(i&1))?3800:200;
        Line_Update(&line,raw); CHECK(line.mask==mask);
    }
    Line_BeginCalibration(&line);
    for(mask=0;mask<60;++mask) Line_Update(&line,raw);
    CHECK(!Line_EndCalibration(&line) && !line.valid && !line.position_confidence);
}
static void Mixer(void) {
    ControlState s; ControlParameters p; unsigned i,dt,b; int sign;
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55); p.kd=0;
        Control_Step(&s,&p,sign*1506,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.active_eye==sign*4 && s.corner_pending_dir==sign);
        CHECK(s.mode==CONTROL_TRACK && s.target==35 && s.speed==35);
        CHECK(sign>0?(s.left==350 && s.right==-297):(s.left==-297 && s.right==350));
        /* Opposite side changes the command in this frame, no timed forced yaw. */
        Control_Step(&s,&p,-sign*1506,1,sign>0?0x1F:0xF8,dt);
        CHECK(s.active_eye==-sign*4 && s.corner_pending_dir==-sign);
        CHECK(sign>0?(s.left==-297 && s.right==350):(s.left==350 && s.right==-297));
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(!s.active_eye && s.target==55 && s.speed>35 && s.speed<=35+0.201f*dt);
        CHECK(s.left>0 && s.right>0 && s.mode==CONTROL_TRACK);
        /* Higher/lower Base cannot bypass the dedicated corner ceiling. */
        for(b=0;b<5;++b) {
            const unsigned bases[5]={20,30,45,55,80};
            unsigned ceiling=bases[b]<35?bases[b]:35;
            Warm(&s,&p,bases[b]); p.kd=0;
            Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,dt);
            CHECK(s.target<=ceiling && s.target>0);
            CHECK(s.left<=(int)ceiling*10 && s.right<=(int)ceiling*10);
            CHECK(s.left>=-(int)ceiling*9 && s.right>=-(int)ceiling*9 && s.left+s.right>0);
        }
        /* Ordinary soft curve receives live PD; speed recovers through Base. */
        Warm(&s,&p,55); p.kd=0;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt);
        CHECK(s.active_eye==sign && s.mode==CONTROL_TRACK && s.target==53);
        CHECK(s.left>0 && s.right>0 && sign*(s.left-s.right)>0);
        CHECK(s.left<=550 && s.right<=550);
        /* D must release reverse after P was assisted, never re-force it. */
        Warm(&s,&p,55); Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
        s.derivative=-sign*240; s.last_error=sign*1500/1000.0f;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
        CHECK(sign>0?s.right>-297:s.left>-297);
        CHECK(sign*s.d_output<0 && s.left+s.right>0);
        /* Conditional integral cannot wind up while the assisted P/wheel clips. */
        Warm(&s,&p,55); p.kp=100; p.ki=2; p.kd=0;
        for(i=0;i<50;++i) {
            Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,dt);
            CHECK(s.integral==0 && s.left+s.right>0);
        }
    }
    /* Amplitude hysteresis and dominance operate on strengths, not mask bits. */
    {
        uint16_t values[8]={0,0,0,700,700,0,0,590};
        Warm(&s,&p,55);
        Control_StepSensors(&s,&p,0,1,0x98,values,2); CHECK(s.active_eye==1);
        values[7]=600; Control_StepSensors(&s,&p,0,1,0x98,values,2); CHECK(s.active_eye==4);
        values[7]=500; Control_StepSensors(&s,&p,0,1,0x98,values,2); CHECK(s.active_eye==4);
        values[7]=450; Control_StepSensors(&s,&p,0,1,0x18,values,2); CHECK(!s.active_eye && !(s.eye_mask&0x80));
        values[0]=700; values[7]=700;
        Control_StepSensors(&s,&p,0,0,0x99,values,2); CHECK(!s.active_eye);
    }
}
static void RecoveryAndGaps(void) {
    ControlState s; ControlParameters p; unsigned i,dt; int sign;
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55);
        for(i=0;i<280/dt;++i) { Control_Step(&s,&p,0,0,0,dt); CHECK(s.mode==CONTROL_GAP && s.left==550 && s.right==550); }
        Control_Step(&s,&p,0,1,0x18,dt); CHECK(s.mode==CONTROL_TRACK && s.target==55);
        Control_Step(&s,&p,sign*1506,1,sign>0?0xF8:0x1F,dt);
        Control_Step(&s,&p,0,0,0,dt); CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        Control_Step(&s,&p,-sign*1506,1,sign>0?0x1F:0xF8,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.target==35 && s.active_eye==-sign*4);
        CHECK(sign*(s.left-s.right)<0); /* New branch owns the first valid frame. */
        for(i=0;i<1300/dt;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Control_Step(&s,&p,0,1,0x18,dt); CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        Warm(&s,&p,55); Control_Step(&s,&p,0,1,0x18,5); CHECK(s.fault==FAULT_TIMING);
    }
    /* Twelve short successive bends with loss/rejoin must not accumulate a false stop. */
    Warm(&s,&p,55);
    for(i=0;i<12;++i) {
        unsigned j; sign=(i&1)?1:-1;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        for(j=0;j<24;++j) Control_Step(&s,&p,sign*1000,1,sign>0?0x30:0x0C,2);
        CHECK(s.fault==FAULT_NONE && s.mode==CONTROL_TRACK);
        CHECK(s.recovery_ms==0 && s.left>0 && s.right>0);
    }
    /* Startup and a changed Max bound both forward and reverse wheel commands. */
    Control_Defaults(&p); Control_Start(&s);
    for(i=0;i<100;++i) {
        Control_Step(&s,&p,3500,1,0x80,2);
        CHECK(s.left<=s.launch*10+1 && s.right>=-s.launch*10-1);
    }
    p.max_percent=10; Control_Step(&s,&p,3500,1,0x80,2);
    CHECK(s.left<=100 && s.right<=100 && s.left>=-100 && s.right>=-100);
    Control_Stop(&s,FAULT_UI); CHECK(!s.eye_mask && !s.active_eye && s.launch==0);
    /* The observed outer side owns search direction even if centroid flickers opposite. */
    Warm(&s,&p,55); Control_Step(&s,&p,1500,1,0xF8,2);
    Control_Step(&s,&p,0,0,0,2);
    Control_Step(&s,&p,1500,1,0x1F,2);
    CHECK(s.mode==CONTROL_ALIGN && s.active_eye==-4 && s.turn_dir==-1);
    Control_Step(&s,&p,0,0,0,2); CHECK(s.mode==CONTROL_TURN && s.turn_dir==-1);
}
static void Adversarial(void) {
    ControlState s; ControlParameters p; unsigned maximum,dt,i; int eye,sign;
    for(maximum=10;maximum<=100;maximum+=10) for(dt=1;dt<=4;++dt) {
        Warm(&s,&p,maximum); p.max_percent=maximum; p.kp=100; p.ki=2; p.kd=1;
        for(i=0;i<96;++i) {
            eye=i%8; sign=eye<4?-1:1;
            Control_Step(&s,&p,(int16_t)((int)eye*1000-3500),1,(uint8_t)(1U<<eye),dt);
            CHECK(s.fault==FAULT_NONE && s.mode==CONTROL_TRACK);
            CHECK(s.left<=(int)maximum*10 && s.right<=(int)maximum*10);
            CHECK(s.left>=-(int)maximum*10 && s.right>=-(int)maximum*10);
            CHECK(s.left+s.right>0); /* Reverse never overpowers its forward partner. */
            CHECK(s.integral>=-5 && s.integral<=5);
            CHECK(sign*s.active_eye>0);
        }
        p.max_percent=0; Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_STOPPED && !s.left && !s.right);
    }
    /* Soft S-bends and coarse center jitter keep running without recovery. */
    Warm(&s,&p,55);
    for(i=0;i<1000;++i) {
        int e=(int)(i%100); e=(e<=50?e:100-e)*24;
        sign=((i/100)&1)?1:-1;
        Control_Step(&s,&p,(int16_t)(e*sign),1,sign>0?0x30:0x0C,2);
        CHECK(s.mode==CONTROL_TRACK && s.left+s.right>0 && !s.brake_count);
        CHECK(s.target>=25 && s.target<=55);
        CHECK(Abs(s.d_output)<=24.001f);
    }
    /* Equal multi-group/T-like patterns cannot select a direction from no history. */
    Control_Defaults(&p); Control_Start(&s);
    Control_Step(&s,&p,0,0,0x81,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_NO_HISTORY);
    Warm(&s,&p,55); p.kp=p.ki=p.kd=0;
    for(i=0;i<20;++i) {
        Control_Step(&s,&p,1500,1,0xF8,2);
        CHECK(s.left==s.right && s.left>0 && s.last_correction==0);
    }
    /* Preserve B38: a valid deadline frame wins; a brief glimpse does not renew budget. */
    Warm(&s,&p,55); s.mode=CONTROL_TURN; s.turn_dir=1; s.recovery_ms=1198;
    Control_Step(&s,&p,1500,1,0x20,2);
    CHECK(s.mode==CONTROL_ALIGN && s.fault==FAULT_NONE && s.recovery_ms==1200);
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Warm(&s,&p,55); s.mode=CONTROL_TURN; s.turn_dir=1; s.recovery_ms=1198;
    for(i=0;i<20;++i) Control_Step(&s,&p,1500,1,0x20,2);
    CHECK(s.mode==CONTROL_TRACK && !s.recovery_ms && s.fault==FAULT_NONE);
    Warm(&s,&p,55); s.mode=CONTROL_TURN; s.turn_dir=1; s.recovery_ms=1100;
    for(i=0;i<100;++i) Control_Step(&s,&p,3000,(i%4)==0,(i%4)==0?0x80:0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    /* Balanced fragments get only the existing bounded continuation, never a fresh side. */
    Warm(&s,&p,55);
    for(i=0;i<16;++i) {
        Control_Step(&s,&p,30000,0,0x99,2);
        CHECK(s.mode==CONTROL_PROBE && s.probe_continue && !s.active_eye);
        CHECK(s.left>0 && s.right>0 && s.speed<=55);
    }
    Control_Step(&s,&p,30000,0,0x99,2); CHECK(!s.probe_continue && s.target==15);
    for(i=0;i<30;++i) Control_Step(&s,&p,30000,0,0x99,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_AMBIGUOUS);
}
void RunGapMemoryTests(void);
void RunRobotTests(void); void RunMenuTests(void); void RunBlackboxTests(void);
void RunMotorTests(void); void RunLogDriverTests(void); void RunBoardTests(void);
void test_entry(void) {
    ControlParameters p; Control_Defaults(&p);
    CHECK(p.kp==12 && p.ki==0 && Abs(p.kd-0.1f)<0.00001f && p.base_percent==55 && p.max_percent==85);
    Estimator(); Mixer(); RecoveryAndGaps(); Adversarial(); RunGapMemoryTests(); test_done();
}
void test_entry_io(void) {
    RunRobotTests(); RunMenuTests(); RunBlackboxTests(); RunMotorTests(); RunLogDriverTests(); RunBoardTests(); test_done();
}
