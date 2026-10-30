#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=180000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Defaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
    p->kd=0;
}
void RunB28RestoreTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); s.last_error=sign*1.5f;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        /* Restore B28 six-ms bounded gain, rather than unconfirmed shaping. */
        CHECK(sign*s.last_correction>17.99f && sign*s.last_correction<18.01f);
        CHECK(s.corner_gain==0 && !s.corner_pending_ms);
        for(i=0;i<2;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*s.last_correction>19.43f && sign*s.last_correction<19.45f);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        CHECK((sign>0?s.right:s.left)<=10);
        CHECK((s.left+s.right)<=900); /* No B31 extra common above Base45. */
        CHECK(s.mode==CONTROL_TRACK && s.target==45);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.recovery_arc && s.left>0 && s.right>0);
        for(i=0;i<13;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.recovery_arc && s.recovery_ms==26);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(!s.recovery_arc && s.recovery_ms==28);
        CHECK(s.left==sign*500 && s.right==-sign*500);
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); p.kp=100; p.ki=2;
        for(i=0;i<10;++i) Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.integral==0 && s.left>=0 && s.right>=0);
        CHECK(s.left+s.right<=900 && s.left<=850 && s.right<=850);
        Control_Stop(&s,FAULT_UI);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
}
