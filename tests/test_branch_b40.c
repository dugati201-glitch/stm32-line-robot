#include "control.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=250000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p,unsigned base) {
    unsigned i; Control_Defaults(p); p->base_percent=p->straight_percent=base;
    Control_Start(s); s->speed=base;
    for(i=0;i<60;++i) Control_Step(s,p,88,1,0x18,2);
}
void test_entry(void) {
    ControlState s; ControlParameters p; unsigned i,dt; int sign;
    /* Log-inspired sequence: missing 2ms samples are synthetic, not a plant replay. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55);
        Control_Step(&s,&p,88,0,sign>0?0x98:0x19,2);
        Control_Step(&s,&p,sign*1506,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*s.derivative>50); /* B39 resets this to zero. */
        Control_Step(&s,&p,sign*1565,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,sign*1624,1,sign>0?0xF8:0x1F,2);
        CHECK(sign>0?s.right<180:s.left<180);
        CHECK(s.mode==CONTROL_TRACK && s.left+s.right>0 && !s.brake_count);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55); p.kd=0;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
        CHECK(sign*s.last_correction>42.5f && sign*s.last_correction<42.7f);
        CHECK(s.left<=850 && s.right<=850 && s.left+s.right>0);
        Control_Step(&s,&p,sign*1400,1,sign>0?0xF8:0x1F,dt);
        CHECK(sign*s.last_correction<28); /* Clear return releases branch bias. */
        Warm(&s,&p,30); p.kd=0;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
        CHECK(sign*s.last_correction>26.9f && sign*s.last_correction<27.1f);
        Warm(&s,&p,55); p.kp=p.ki=p.kd=0;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left==s.right && s.left>400 && s.left<550);
        Warm(&s,&p,55); p.kd=0;
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x30:0x0C,dt);
        CHECK(sign*s.last_correction>17.9f && sign*s.last_correction<18.1f);
    }
    /* A white dash or an old fragmented loss must not retain a stale D kick. */
    Warm(&s,&p,55); Control_Step(&s,&p,0,0,0,2);
    Control_Step(&s,&p,500,1,0x10,2); CHECK(s.derivative==0);
    Warm(&s,&p,55);
    for(i=0;i<3;++i) Control_Step(&s,&p,88,0,0x98,2);
    Control_Step(&s,&p,1506,1,0xF8,2); CHECK(s.derivative==0);
    test_done();
}
