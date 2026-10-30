#include "control.h"
#include "control_tuning.h"
void B45_Control_Step(ControlState *,const ControlParameters *,int16_t,uint8_t,uint8_t,uint32_t);
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=310000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Seed(ControlState *s,ControlParameters *p,unsigned base,int sign,float error) {
    Control_Defaults(p);p->base_percent=p->straight_percent=base;
    Control_Start(s);s->history=1;s->speed=(float)base;s->last_error=sign*error;
}
void test_entry(void) {
    ControlState s,old;ControlParameters p;unsigned base,dt,i;int sign;
    const int errors[7]={0,550,551,1000,1650,2500,3500};
    const float target55[7]={55,55,54.9955f,52.975f,50.05f,49,49};
    for(sign=-1;sign<=1;sign+=2) for(dt=1;dt<=4;++dt) {
        for(base=30;base<=60;base+=5) for(i=0;i<7;++i) {
            float wanted=(float)base,drop;
            Seed(&s,&p,base,sign,errors[i]*0.001f);p.kp=p.kd=0;
            if(base>49 && errors[i]>550) {
                wanted=base-(errors[i]-550)*0.0045f;
                if(wanted<49) wanted=49;
            }
            s.speed_guard_ms=300;s.stable_ms=0;
            Control_Step(&s,&p,sign*errors[i],1,0x18,dt);
            CHECK(Abs(s.target-wanted)<0.001f);
            if(base==55) CHECK(Abs(s.target-target55[i])<0.001f);
            drop=base-wanted;if(drop>0.6f*dt)drop=0.6f*dt;
            CHECK(Abs(s.speed-(base-drop))<0.001f);
            CHECK(s.mode==CONTROL_TRACK && !s.brake_count);
            Control_Step(&s,&p,0,1,0x18,dt);
            CHECK(s.target==base); /* No stable/guard wait for the centered target. */
        }
        Seed(&s,&p,55,sign,0);s.speed=49;p.kp=p.kd=0;
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(Abs(s.speed-(49+0.22f*dt))<0.001f);
        Seed(&s,&p,55,sign,0);s.speed=49;s.post_turn_ms=200;
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(Abs(s.speed-(49+0.26f*dt))<0.001f);
        /* Isolated outer cue selects recovery on the first white update. */
        Seed(&s,&p,55,sign,2.5f);old=s;p.kd=0;
        Control_Step(&s,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        B45_Control_Step(&old,&p,sign*2500,1,sign>0?0x40:0x02,dt);
        CHECK(s.corner_pending_ms==200 && s.corner_pending_dir==sign && s.last_side==sign);
        CHECK(!old.corner_pending_ms);
        Control_Step(&s,&p,0,0,0,dt);B45_Control_Step(&old,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign && !s.brake_count);
        CHECK(s.left==(sign>0?700:-330) && s.right==(sign>0?-330:700));
        CHECK(old.mode==CONTROL_PROBE);
        /* Centered ALIGN completion must preserve inward D and integral history. */
        Seed(&s,&p,55,sign,0.9f);s.mode=CONTROL_ALIGN;s.turn_dir=sign;
        s.reacquire_ms=6-dt;s.derivative=-sign*30;s.integral=1.0f;p.ki=0.1f;
        old=s;
        Control_Step(&s,&p,sign*800,1,sign>0?0x30:0x0C,dt);
        B45_Control_Step(&old,&p,sign*800,1,sign>0?0x30:0x0C,dt);
        CHECK(s.mode==CONTROL_TRACK && old.mode==CONTROL_TRACK);
        CHECK(sign*s.derivative < -20 && s.integral>0.9f && s.d_output*sign<0);
        CHECK(old.derivative==0 && old.integral<0.01f);
        CHECK(s.target==55); /* ALIGN does not receive TRACK curve slowing. */
        /* Sign-mismatched or returning outer data must not create a fresh latch. */
        for(i=0;i<3;++i) {
            Seed(&s,&p,55,sign,i==1?2.501f:2.5f);
            if(i==2)s.derivative=-sign*100;
            Control_Step(&s,&p,sign*2500,1,i==0?(sign>0?0x02:0x40):(sign>0?0x40:0x02),dt);
            CHECK(!s.corner_pending_ms && s.left>=0 && s.right>=0);
        }
    }
    test_done();
}
