#include "control.h"
#include "control_tuning.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) {
    ++test_count; if(!ok) { failure_line=230000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Seed(ControlState *s,ControlParameters *p,int sign) {
    Control_Defaults(p); Control_Start(s); s->history=1;
    s->mode=CONTROL_TURN; s->turn_dir=(int8_t)sign;
    s->speed=50; s->left=sign*500; s->right=-sign*500;
    s->last_error=sign*3.0f; s->recovery_ms=100;
}
void test_entry(void) {
    ControlState s; ControlParameters p;
    unsigned i,j,dt; int sign;
    /* Current valid data must win at the old recovery deadline. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign); s.recovery_ms=1200-dt;
        Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.fault==FAULT_NONE && s.left>0 && s.right>0);
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        /* A genuine moderate curve does not have to become dead center. */
        Seed(&s,&p,sign);
        for(i=0;i<1400/dt;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_TRACK && s.fault==FAULT_NONE && s.left>0 && s.right>0);
        CHECK(s.target>50.724f && s.target<50.726f && !s.recovery_ms);
    }
    /* Many separate corners, with visible travel, exceed one old budget. */
    Control_Defaults(&p); Control_Start(&s); s.speed=55;
    for(i=0;i<60;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(j=0;j<12;++j) {
        sign=(j&1)?-1:1;
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        for(i=0;i<81;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        for(i=0;i<15;++i) Control_Step(&s,&p,500,1,0x10,2);
        CHECK(s.mode==CONTROL_TRACK && s.fault==FAULT_NONE);
    }
    /* Brief glimpses/edge-only data never make an endless recovery legal. */
    Seed(&s,&p,1);
    for(i=0;i<750;++i) Control_Step(&s,&p,1500,(i%8)<7,(i%8)<7?0x20:0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Seed(&s,&p,-1);
    for(i=0;i<650;++i) Control_Step(&s,&p,-3500,1,0x01,2);
    CHECK(s.mode==CONTROL_ALIGN);
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    /* Visible corner assistance starts on arrival, no extra D/forced pivot. */
    for(sign=-1;sign<=1;sign+=2) {
        Control_Defaults(&p); p.kd=0; Control_Start(&s);
        s.history=1; s.speed=55; s.last_error=sign*1.5f;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*s.last_correction>28.355f && sign*s.last_correction<28.357f);
        CHECK(s.mode==CONTROL_TRACK && !s.corner_pending_ms && s.left>0 && s.right>0);
        CHECK(!s.brake_count && Abs(s.target-50.725f)<0.001f);
        Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,2);
        CHECK(Abs(s.corner_gain-0.08f)<0.0001f); /* No carry across sides. */
        Control_Step(&s,&p,500,1,0x10,2);
        CHECK(!s.corner_gain && s.mode==CONTROL_TRACK);
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed;
        /* Release exactly after continuous inner-line evidence, preserving D. */
        Seed(&s,&p,sign); s.recovery_ms=1200-dt;
        elapsed=0;
        do {
            Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt); elapsed+=dt;
            CHECK(s.mode!=CONTROL_FAULT && s.left>0 && s.right>0);
            if(elapsed<CONTROL_RECOVERY_VISIBLE_MS) CHECK(s.recovery_ms==1200 && s.target==55);
        } while(elapsed<CONTROL_RECOVERY_VISIBLE_MS);
        CHECK(s.mode==CONTROL_TRACK && s.target==55 && !s.recovery_ms); /* Final ALIGN output. */
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode!=CONTROL_FAULT && s.recovery_ms==dt);
        /* Evidence at 39ms or less followed by invalid cannot release budget. */
        Seed(&s,&p,sign); s.recovery_ms=1200-dt;
        for(i=0;i<(CONTROL_RECOVERY_VISIBLE_MS-1)/dt;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Seed(&s,&p,sign);
        for(i=0;i<CONTROL_RECOVERY_VISIBLE_MS/dt+1;++i)
            Control_Step(&s,&p,sign*(1500+(int)i*10),1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_TRACK && sign*s.derivative>0 && s.d_output*sign>0);
        /* New opposite corner earns a separate budget only after completed
         * ALIGN, continuous travel, and the four-ms directional latch. */
        Seed(&s,&p,sign); s.mode=CONTROL_TRACK; s.stable_ms=0;
        s.post_turn_ms=200; s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
        for(i=0;i<(CONTROL_NEW_CORNER_TRACK_MS+dt-1)/dt;++i) Control_Step(&s,&p,0,1,0x18,dt);
        elapsed=0;
        do {
            Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,dt); elapsed+=dt;
            if(elapsed<CONTROL_SHAPE_CONFIRM_MS) CHECK(s.post_turn_ms && s.recovery_ms>900);
        } while(elapsed<CONTROL_SHAPE_CONFIRM_MS);
        CHECK(s.mode==CONTROL_TRACK && !s.post_turn_ms && !s.recovery_ms);
        CHECK(s.corner_pending_dir==-sign && s.last_side==-sign);
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==-sign && !s.recovery_arc);
        /* Same-side cue, short travel, incomplete ALIGN, and invalid-only
         * retargets each retain the original bounded episode. */
        for(j=0;j<4;++j) {
            Seed(&s,&p,sign); s.mode=j==2?CONTROL_ALIGN:CONTROL_TRACK;
            s.post_turn_ms=200; s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
            elapsed=0;
            if(j!=1 && j!=2) for(i=0;i<(CONTROL_NEW_CORNER_TRACK_MS+dt-1)/dt;++i) {
                Control_Step(&s,&p,0,1,0x18,dt); elapsed+=dt;
            }
            for(i=0;i<(CONTROL_SHAPE_CONFIRM_MS+dt-1)/dt;++i) {
                int direction=j==0?sign:-sign;
                Control_Step(&s,&p,direction*1500,j!=3,
                    j==3?(direction>0?0xE8:0x17):(direction>0?0xF8:0x1F),dt);
                elapsed+=dt;
            }
            if(j==0) CHECK(!s.recovery_ms && !s.post_turn_ms);
            else CHECK(s.recovery_ms==900+elapsed && s.mode!=CONTROL_FAULT);
        }
        /* One invalid frame breaks the visible-travel qualification. */
        Seed(&s,&p,sign); s.mode=CONTROL_TRACK;
        s.post_turn_ms=200; s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
        elapsed=0;
        for(i=0;i<(CONTROL_NEW_CORNER_TRACK_MS+dt-1)/dt;++i) { Control_Step(&s,&p,0,1,0x18,dt); elapsed+=dt; }
        Control_Step(&s,&p,0,0,0,dt); elapsed+=dt;
        for(i=0;i<(CONTROL_SHAPE_CONFIRM_MS+dt-1)/dt;++i) {
            Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,dt); elapsed+=dt;
        }
        CHECK(s.recovery_ms==900+elapsed && s.post_turn_ms);
    }
    /* Coincident legacy post-turn/recovery flags count a frame once. */
    Seed(&s,&p,1); s.mode=CONTROL_ALIGN; s.post_turn_ms=200; s.recovery_ms=900;
    Control_Step(&s,&p,1500,1,0x20,2);
    CHECK(s.recovery_ms==902);
    /* Tiny Max, high I/P, normal soft curves, and fault latch retain bounds. */
    Control_Defaults(&p); p.kp=100; p.ki=2; p.max_percent=10;
    Control_Start(&s); s.history=1; s.speed=10;
    for(i=0;i<20;++i) {
        Control_Step(&s,&p,2000,1,0xF8,2);
        CHECK(s.left>=0 && s.right>=0 && s.left<=100 && s.right<=100 && !s.integral);
    }
    Control_Stop(&s,FAULT_ADC); Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && s.fault==FAULT_ADC && !s.left && !s.right);
    test_done();
}
