#include "control.h"
#include "control_tuning.h"
void B43_Control_Start(ControlState *s);
void B43_Control_Step(ControlState *s,const ControlParameters *p,int16_t e,uint8_t v,uint8_t m,uint32_t dt);
volatile unsigned test_count,failure_line;
volatile int grip_trace[4][5];
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=300000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Seed(ControlState *s,ControlParameters *p,ControlMode mode,int sign,float duty) {
    Control_Defaults(p); Control_Start(s); s->mode=mode; s->history=1;
    s->turn_dir=(int8_t)sign; s->speed=duty; s->last_error=sign*2.5f;
}
void test_entry(void) {
    ControlState s,old; ControlParameters p; unsigned i,j,dt,maximum; int sign,next;
    Control_Defaults(&p);
    CHECK(p.base_percent==55 && p.straight_percent==55 && p.max_percent==85);
    CHECK(p.kp==12 && !p.ki && p.kd==0.9f && p.align_ms==6);
    CHECK(CONTROL_D_FILTER_SECONDS==0.01f && CONTROL_CORNER_D_MAX_PERCENT==28);
    for(sign=-1;sign<=1;sign+=2) for(dt=1;dt<=4;++dt) {
        /* Isolated outer eyes work without pending/shape/confirmed drive direction. */
        Seed(&s,&p,CONTROL_TRACK,sign,55); p.kd=0;
        B43_Control_Start(&old); old.history=1; old.speed=55; old.last_error=sign*2.5f;
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            B43_Control_Step(&old,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            CHECK(s.mode==CONTROL_TRACK && !s.corner_pending_ms && !s.drive_corner_dir);
            CHECK(s.left>=-240 && s.right>=-240 && s.left<=850 && s.right<=850);
            CHECK(old.left>=0 && old.right>=0 && !old.corner_gain);
        }
        CHECK(sign>0?s.right<0:s.left<0);
        if(sign>0 && dt==2) {
            grip_trace[0][0]=0; grip_trace[0][1]=old.left; grip_trace[0][2]=old.right;
            grip_trace[0][3]=s.left; grip_trace[0][4]=s.right;
        }
        /* One pitch-thousandth return releases now, even with outward filtered D. */
        s.derivative=sign*100; p.kd=0.9f;
        Control_Step(&s,&p,sign*2499,1,sign>0?0x40:0x02,dt);
        CHECK(sign*s.derivative>0 && s.left>=0 && s.right>=0 && !s.corner_gain);
        if(sign>0 && dt==2) {
            grip_trace[1][0]=1; grip_trace[1][3]=s.left; grip_trace[1][4]=s.right;
        }
        /* Filtered inward D blocks grip even when the raw step is outward. */
        Seed(&s,&p,CONTROL_TRACK,sign,55); s.derivative=-sign*100;
        Control_Step(&s,&p,sign*2501,1,sign>0?0x40:0x02,dt);
        CHECK(sign*s.derivative<0 && s.left>=0 && s.right>=0 && !s.corner_gain);
        /* ALIGN never receives tight/common reduction or corner gain. */
        Seed(&s,&p,CONTROL_ALIGN,sign,55); p.kd=0;
        for(i=0;i<10;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0 && !s.corner_gain);
        CHECK(s.speed==55 && s.target==55);
        /* Observed single-eye authority is present after TTL expiry as well. */
        Seed(&s,&p,CONTROL_TRACK,sign,55); p.kd=0; s.corner_pending_ms=dt; s.drive_corner_dir=(int8_t)sign;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(!s.corner_pending_ms && (sign>0?s.right<0:s.left<0));
        /* Wrong-side masks, central-only data, low duty and tiny Max grant no reverse. */
        for(j=0;j<4;++j) {
            Seed(&s,&p,CONTROL_TRACK,sign,j==2?30:55); p.kd=0;
            if(j==2) p.base_percent=p.straight_percent=30;
            if(j==3) p.max_percent=10;
            for(i=0;i<20;++i) Control_Step(&s,&p,sign*2500,1,
                j==0?(sign>0?0x02:0x40):j==1?0x18:(sign>0?0x40:0x02),dt);
            CHECK(s.left>=0 && s.right>=0 && !s.corner_gain);
        }
        /* Ordinary gap behavior and valid-before-timeout remain bounded. */
        Seed(&s,&p,CONTROL_TURN,sign,55); s.recovery_ms=1200-dt;
        Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.fault==FAULT_NONE);
        Control_Step(&s,&p,0,0,0,dt); CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        /* Carry is capped at55 without boosting low-duty approaches. */
        for(j=0;j<2;++j) {
            Seed(&s,&p,CONTROL_TURN,sign,j?5:80);
            Control_Step(&s,&p,0,0,0,dt);
            CHECK(s.left==(sign>0?680:-300) && s.right==(sign>0?-300:680));
            CHECK(s.speed==(j?5:55));
            Control_Step(&s,&p,0,1,0x18,dt);
            CHECK(s.target==55 && (j?s.speed<6:s.speed==55));
        }
    }
    /* Smooth position gain is numeric and symmetric, not a corner threshold. */
    for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,CONTROL_TRACK,sign,55); p.kd=0; s.last_error=sign*0.5f;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        CHECK(Abs(s.last_correction-sign*6.3f)<0.001f);
    }
    /* Low P avoids actuator clipping, exposing20% vs35% common reduction directly. */
    Seed(&s,&p,CONTROL_TRACK,1,55); p.kp=5; p.kd=0;
    Control_Step(&s,&p,2500,1,0x40,2);
    CHECK(Abs((s.left+s.right)/20.0f-44)<0.11f);
    grip_trace[2][0]=2; grip_trace[2][3]=s.left; grip_trace[2][4]=s.right;
    Seed(&s,&p,CONTROL_TRACK,1,55); p.kp=5; p.kd=0;
    s.shape_ms=4; s.shape_dir=1; s.corner_pending_dir=s.drive_corner_dir=1; s.corner_pending_ms=100;
    Control_Step(&s,&p,2500,1,0xF8,2);
    CHECK(Abs((s.left+s.right)/20.0f-35.75f)<0.11f);
    grip_trace[3][0]=3; grip_trace[3][3]=s.left; grip_trace[3][4]=s.right;
    for(maximum=10;maximum<=100;maximum+=5) for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,CONTROL_TRACK,sign,55); p.max_percent=maximum; p.kp=100; p.ki=2;
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
            CHECK(s.left<=(int)maximum*10 && s.right<=(int)maximum*10);
            CHECK(s.left>=-240 && s.right>=-240 && !s.integral);
        }
    }
    /* New same/opposite corner needs8ms narrow travel before4ms broad confirmation. */
    for(sign=-1;sign<=1;sign+=2) for(next=-1;next<=1;next+=2) for(j=0;j<3;++j) {
        Seed(&s,&p,CONTROL_TRACK,sign,55); s.post_turn_ms=200; s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
        for(i=0;i<(j==1?7:8);++i) Control_Step(&s,&p,0,1,0x18,1);
        if(j==2) Control_Step(&s,&p,0,0,0,1);
        for(i=0;i<4;++i) Control_Step(&s,&p,next*1500,1,next>0?0xF8:0x1F,1);
        if(!j) CHECK(!s.post_turn_ms && !s.recovery_ms);
        else CHECK(s.post_turn_ms && s.recovery_ms>=900);
    }
    test_done();
}
