#include "legacy_profile.h"
#include "control.h"
#include "control_tuning.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=210000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    TestLegacyDefaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void test_entry(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); p.kd=0; s.last_error=sign*1.5f;
        for(i=0;i<16;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        /* Stronger visible-corner P, not a stronger pivot after losing line. */
        CHECK(sign*s.last_correction>47.189f && sign*s.last_correction<47.191f);
        CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.left>=-280 && s.right>=-280 && s.left+s.right>0);
        CHECK(s.left+s.right<=900 && !s.brake_count);
        Control_Step(&s,&p,sign*1000,1,0x18,2);
        CHECK(s.corner_gain==0 && Abs(s.last_correction-sign*13.2f)<0.001f);
        Warm(&s,&p); p.kd=0;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x30:0x0C,2);
        CHECK(s.corner_gain==0 && Abs(s.last_correction-sign*20.7f)<0.001f);
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0,budget;
        Warm(&s,&p);
        s.mode=CONTROL_TURN; s.turn_dir=(int8_t)sign;
        s.speed=50; s.left=sign*500; s.right=-sign*500; s.recovery_ms=200;
        do {
            Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt); elapsed+=dt;
            if(elapsed<6) CHECK(s.mode==CONTROL_ALIGN && s.target==45);
        } while(elapsed<6);
        CHECK(s.mode==CONTROL_TRACK && s.target==45);
        CHECK(s.post_turn_ms && s.recovery_ms==200+elapsed);
        CHECK(s.speed==45); /* Ramp, never jump to Base. */
        budget=s.recovery_ms;
        for(i=0;i<6/dt+1;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        CHECK(s.recovery_ms==budget+(6/dt+1)*dt && !s.recovery_arc);
        for(i=0;i<1300/dt;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    /* A broad central patch or a transient center is not a fast exit. */
    Warm(&s,&p); s.mode=CONTROL_TURN; s.turn_dir=1; s.recovery_ms=100; s.speed=50;
    for(i=0;i<3;++i) Control_Step(&s,&p,0,1,0x3C,2);
    CHECK(s.mode==CONTROL_ALIGN && s.target==45);
    Warm(&s,&p); s.mode=CONTROL_TURN; s.turn_dir=1; s.recovery_ms=100; s.speed=50;
    for(i=0;i<2;++i) Control_Step(&s,&p,0,1,0x18,2);
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_TURN && s.recovery_ms==106);
    /* Keep D's counter-steering when the line reaches center; release extra
     * corner P, rather than resetting damping at the fast handoff. */
    Warm(&s,&p); s.mode=CONTROL_TURN; s.turn_dir=1; s.speed=50;
    Control_Step(&s,&p,500,1,0x10,2);
    Control_Step(&s,&p,500,1,0x10,2);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.derivative<0 && s.last_correction<0);
    CHECK(s.corner_gain==0 && s.left>0 && s.right>0 && s.target==45);
    Control_Step(&s,&p,2000,1,0xF8,2);
    CHECK(s.target==45 && !s.fast_exit); /* Large new error restores guard. */
    /* A user-selected 4ms legacy handoff can still earn the 6ms fast release
     * in post-turn TRACK. Do not force a longer legacy wait. */
    Warm(&s,&p); p.align_ms=4; s.mode=CONTROL_TURN; s.turn_dir=1; s.speed=50;
    Control_Step(&s,&p,0,1,0x18,2);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.target==45);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK && s.target==45 && s.fast_exit);
    /* Clipping and antiwindup remain valid at extreme gain and reduced Max. */
    Warm(&s,&p); p.kp=100; p.ki=2;
    for(i=0;i<12;++i) Control_Step(&s,&p,2000,1,0xF8,2);
    CHECK(s.integral==0 && s.left<=850 && s.right<=850);
    p.max_percent=10; Control_Step(&s,&p,2000,1,0xF8,2);
    CHECK(s.left>=0 && s.right>=0 && s.left<=100 && s.right<=100);
    Control_Stop(&s,FAULT_UI);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    test_done();
}
