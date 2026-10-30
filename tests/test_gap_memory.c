#include "legacy_profile.h"
#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=120000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    TestLegacyDefaults(p); Control_Start(s);
    /* Already rolling; earn 120 ms of real centered guide history. Startup
     * ramp has independent coverage in the fixed-Base/controller suite. */
    s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunGapMemoryTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i;
    int sign;
    /* A single central-eye steering kick is not frozen across a white dash. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
        {
            int live=s.left-s.right, l,r;
            Control_Step(&s,&p,0,0,0,2);
            l=s.left; r=s.right;
            CHECK(Abs(l-r)<Abs(live)*0.2f && l>0 && r>0);
            for(i=0;i<99;++i) {
                Control_Step(&s,&p,0,0,0,2);
                CHECK(s.mode==CONTROL_GAP && s.left==l && s.right==r);
            }
            Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,2);
            CHECK(s.mode==CONTROL_TRACK && s.derivative==0 && s.left>0 && s.right>0);
        }
        Control_Stop(&s,FAULT_ADC);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right && !s.recovery_arc);
    }
}
