#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=170000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Seed(ControlState *s,ControlParameters *p,int sign)
{
    Control_Defaults(p); Control_Start(s);
    s->history=1; s->mode=CONTROL_TURN; s->turn_dir=(int8_t)sign;
    s->speed=45; s->left=sign*500; s->right=-sign*500;
    s->last_error=sign*3.0f; s->recovery_ms=100;
}
void RunRecoveryExitTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign);
        /* A single line held at 1.5 pitches is still valid tracking.
         * B31 never completes ALIGN and eventually faults despite valid data. */
        for(i=0;i<650;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,2);
        CHECK(s.mode==CONTROL_TRACK && s.fault==FAULT_NONE && s.left>0 && s.right>0);
        Seed(&s,&p,sign);
        for(i=0;i<20;++i) Control_Step(&s,&p,sign*(1500+i*10),1,sign>0?0x20:0x04,2);
        CHECK(s.mode==CONTROL_TRACK && sign*s.derivative>1.0f);
    }
    /* Fresh valid data at the old deadline gets PD authority in that frame;
     * it must still earn a continuous visible window to release the budget. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign); s.recovery_ms=1200-dt;
        Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>0 && s.right>0);
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Seed(&s,&p,sign); s.recovery_ms=1200-dt;
        for(i=0;i<44/dt+1;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0x20:0x04,dt);
        CHECK(s.mode==CONTROL_TRACK && s.recovery_ms==0 && s.post_turn_ms);
        CHECK(s.recovery_arc_speed==0); /* Cannot renew an old rolling arc. */
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode!=CONTROL_FAULT && s.recovery_ms==dt);
    }
    /* Brief glimpses or valid outer-edge samples never qualify as sustained
     * inner-line tracking; the next invalid frame still enforces timeout. */
    Seed(&s,&p,1);
    for(i=0;i<700;++i) Control_Step(&s,&p,1500,(i%10)<9,(i%10)<9?0x20:0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Seed(&s,&p,-1);
    for(i=0;i<650;++i) Control_Step(&s,&p,-3500,1,0x01,2);
    CHECK(s.mode==CONTROL_ALIGN); /* Valid PD, but not settled tracking. */
    Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
}
