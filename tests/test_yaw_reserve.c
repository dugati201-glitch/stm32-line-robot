#include "control.h"
#include "control_tuning.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) {
    ++test_count; if(!ok) { failure_line=240000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p,unsigned base) {
    unsigned i;
    Control_Defaults(p); p->base_percent=p->straight_percent=(uint16_t)base;
    Control_Start(s); s->speed=(float)base;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
static void Cue(ControlState *s,ControlParameters *p,int sign,unsigned dt) {
    unsigned elapsed=0;
    do {
        Control_Step(s,p,sign*1500,1,sign>0?0xF8:0x1F,dt); elapsed+=dt;
    } while(elapsed<6);
}
void test_entry(void) {
    ControlState s; ControlParameters p;
    int sign; unsigned i,dt,b;
    const unsigned bases[2]={45,55};
    /* Same already-observed corner, now leaving the central eyes: steering
     * must have useful authority before the estimator becomes invalid. */
    for(b=0;b<2;++b) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,bases[b]); Cue(&s,&p,sign,2);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        CHECK(s.mode==CONTROL_TRACK && s.fault==FAULT_NONE);
        CHECK(sign>0?s.right<0:s.left<0);
        CHECK(sign>0?s.left>0:s.right>0);
        CHECK(s.left+s.right>0 && !s.brake_count && s.target==bases[b]);
    }
    /* Last known corner alone cannot force a timed signed output on valid
     * center/soft-curve data; D continues to damp a returning line. */
    Warm(&s,&p,55); Cue(&s,&p,1,2);
    for(i=0;i<20;++i) Control_Step(&s,&p,2500,1,0x40,2);
    CHECK(s.right<0 && s.left<=850 && s.right>=-240);
    Control_Step(&s,&p,1500,1,0x20,2);
    CHECK(s.left>=0 && s.right>=0);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.left>=0 && s.right>=0 && s.mode==CONTROL_TRACK);
    /* Even when filtered D still points outward, a raw returning frame must
     * release only the new reserve; it must not wait for D to change sign. */
    Warm(&s,&p,55); Cue(&s,&p,1,2);
    Control_Step(&s,&p,2500,1,0x40,2);
    Control_Step(&s,&p,2400,1,0x40,2);
    CHECK(s.derivative>0 && s.left>=0 && s.right>=0 && !s.corner_gain);
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        float before;
        Warm(&s,&p,55); Cue(&s,&p,sign,dt);
        for(i=0;i<80/dt;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            CHECK(s.mode==CONTROL_TRACK && s.left+s.right>0);
            CHECK(s.left>=-240 && s.right>=-240 && s.left<=850 && s.right<=850);
        }
        CHECK(sign>0?s.right==-240:s.left==-240);
        /* Returning line releases signed reserve immediately, before a slow
         * planner can mask its inward motion. D/filter remain active. */
        Control_Step(&s,&p,sign*2400,1,sign>0?0x40:0x02,dt);
        CHECK(s.left>=0 && s.right>=0 && s.corner_gain==0);
        CHECK(sign*s.derivative<0);
        /* Small returning step may leave raw D outward; eligibility is raw
         * filtered direction, not a claim that yaw has already settled. */
        Warm(&s,&p,55); Cue(&s,&p,sign,dt);
        for(i=0;i<80/dt;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.left>=0 && s.right>=0 && !s.corner_gain);
        /* B45 grants current outer-eye authority without a confirmed branch. */
        Warm(&s,&p,55);
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            CHECK(s.corner_gain>0 && (sign>0?s.right<0:s.left<0));
        }
        /* Startup/low Base and low current Max never inherit signed reserve. */
        Warm(&s,&p,30); Cue(&s,&p,sign,dt);
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            CHECK(s.left>=0 && s.right>=0 && s.target==30 && !s.corner_gain);
        }
        Warm(&s,&p,55); Cue(&s,&p,sign,dt); p.max_percent=10;
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(s.left>=0 && s.right>=0 && s.left<=100 && s.right<=100);
        /* A confirmed direction does not supply steering when user gains are
         * zero; reserve is a bound, never a fixed pivot command. */
        Warm(&s,&p,55); Cue(&s,&p,sign,dt); p.kp=p.kd=p.ki=0;
        for(i=0;i<10;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(s.left==s.right && s.left>0 && s.left<550 && s.last_correction==0);
        /* I uses the expanded directional saturation bound without winding up. */
        Warm(&s,&p,55); Cue(&s,&p,sign,dt); p.kp=100; p.ki=2;
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
            CHECK(s.integral==0 && s.left+s.right>0);
            CHECK(s.left>=-240 && s.right>=-240 && s.left<=850 && s.right<=850);
        }
        /* Lower the allowed Base while a previous high duty is still stored. */
        p.base_percent=20;
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(s.left>=0 && s.right>=0);
        /* Expired cue or changing side cannot retain the old extra gain. */
        Warm(&s,&p,55); Cue(&s,&p,sign,dt);
        for(i=0;i<220/dt+1;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(!s.corner_pending_ms && !s.drive_corner_dir && s.corner_gain>0 && (sign>0?s.right<0:s.left<0));
        Warm(&s,&p,55); Cue(&s,&p,sign,dt);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        before=s.recovery_ms;
        /* Loss retains B38's bounded direct recovery, not a renewed arc. */
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_TURN && !s.recovery_arc && s.recovery_ms==before);
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
        Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    /* Hysteresis is not a timed yaw latch: before confirmation or after the
     * observed side changes, the normal signed-free law owns valid samples. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55);
        Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(!s.corner_pending_ms && (sign>0?s.right<0:s.left<0));
        Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_pending_ms && s.corner_pending_dir==sign);
        Control_Step(&s,&p,sign*2500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_pending_dir==sign);
        for(i=0;i<5;++i) Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
        CHECK(sign>0?s.right<0:s.left<0);
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x02:0x40,2);
        CHECK((sign>0?s.left<0:s.right<0) && s.corner_gain>0);
        /* A different broad shape starts its own ramp, not old-side gain. */
        Control_Step(&s,&p,-sign*2500,1,sign>0?0x1F:0xF8,2);
        CHECK(s.left>=-240 && s.right>=-240 && (sign>0?s.left<0:s.right<0) && s.corner_gain<=0.081f);
    }
    /* Max and fractional duty/position reserve bound all signed requests;
     * the faster D filter may release reserve on an inward jitter frame. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55); Cue(&s,&p,sign,2);
        for(i=0;i<64;++i) {
            int error=sign*((i&1U)?2520:2480);
            Control_Step(&s,&p,(int16_t)error,1,sign>0?0x40:0x02,2);
            if(sign*s.derivative<-CONTROL_TIGHT_INWARD_RATE_PITCH_PER_SECOND)
                CHECK(s.left>=0 && s.right>=0);
            else CHECK(s.left>=-240 && s.right>=-240);
            CHECK(s.left+s.right>0 && s.mode==CONTROL_TRACK);
        }
    }
    for(b=0;b<4;++b) for(sign=-1;sign<=1;sign+=2) {
        const unsigned maxes[4]={20,40,60,100};
        unsigned maximum=maxes[b];
        Warm(&s,&p,maximum<55?maximum:55); p.max_percent=(uint16_t)maximum;
        Cue(&s,&p,sign,2); p.kp=100;
        for(i=0;i<10;++i) {
            Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,2);
            CHECK(s.left<=(int)maximum*10 && s.right<=(int)maximum*10);
            CHECK(s.left>=-240 && s.right>=-240 && s.left+s.right>0);
        }
    }
    test_done();
}
