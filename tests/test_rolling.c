/* Historical B31/B32 policy tests. Superseded in the default suite by
 * test_b28_restore.c after owner-reported 17.4s vs B28 16.3s regression.
 * Reproduce with the archived B31/B32 source and tuning, not current firmware. */
#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=160000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Defaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunRollingTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        /* No observed corner: keep ordinary PID saturation, not a global
         * inner floor. The positive-drive rule is deliberately scoped. */
        Warm(&s,&p); p.kd=0;
        s.last_error=sign*3.5f;
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        CHECK((sign>0?s.right:s.left)<=10 && !s.corner_pending_ms);
        Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        /* An ordinary corner can cross a short white patch while still
         * rolling. B30 requests a pivot at 12ms and fails this at 20ms. */
        Control_Step(&s,&p,0,0,0,2);
        for(i=0;i<10;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.recovery_arc && s.left>0 && s.right>0);
        CHECK((sign>0?s.right:s.left)>=170);
        Control_Step(&s,&p,sign*1500,1,sign>0?0x60:0x06,2);
        CHECK(s.mode==CONTROL_ALIGN && s.target==45 && s.left>0 && s.right>0);
        CHECK(s.recovery_arc && sign*(s.left-s.right)>0);
        /* A large correction on a confirmed visible corner must preserve
         * positive inner drive. Use headroom, not an unconditional stop/pivot. */
        Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,sign*3500,1,sign>0?0x80:0x01,2);
        CHECK(s.mode==CONTROL_TRACK && s.target==45);
        CHECK((sign>0?s.right:s.left)>=179);
        CHECK((sign>0?s.left:s.right)<=850 && sign*(s.left-s.right)>0);
        CHECK(!s.brake_ms && !s.brake_count);
    }
    /* Every frame can hand control back to visible PD; 80ms is a maximum
     * rolling opportunity, never a mandatory blind turn. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0;
        Warm(&s,&p);
        do {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt);
            elapsed+=dt;
        } while(elapsed<6);
        Control_Step(&s,&p,0,0,0,dt);
        elapsed=0;
        do {
            CHECK(s.left>0 && s.right>0 && s.recovery_arc);
            CHECK(s.left<=850 && s.right<=850);
            Control_Step(&s,&p,0,0,0,dt); elapsed+=dt;
        } while(elapsed<80);
        CHECK(!s.recovery_arc && s.left*sign>0 && s.right*sign<0);
        for(i=0;i<1300/dt;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        /* Still obey a newly lowered Max while rolling a visible corner. */
        Warm(&s,&p); p.ki=2; p.kp=100;
        for(i=0;i<8;++i) Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.integral==0);
        p.max_percent=10;
        Control_Step(&s,&p,sign*2000,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left>0 && s.right>0 && s.left<=100 && s.right<=100);
    }
}
