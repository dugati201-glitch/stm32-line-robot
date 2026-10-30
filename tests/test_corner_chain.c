#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=150000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    Control_Defaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunCornerChainTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,j,dt;
    int sign;
    /* Completed centered handoff, real visible travel, then an opposite
     * confirmed corner: these are different turns, not one endless search.
     * Total time exceeds 1200ms, each individual loss is only 160ms. */
    Warm(&s,&p);
    for(j=0;j<10;++j) {
        sign=(j&1U)?-1:1;
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        for(i=0;i<80;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign);
        for(i=0;i<6;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK);
        for(i=0;i<15;++i) Control_Step(&s,&p,500,1,0x10,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
    }
    /* B33 restores B28: ordinary P acts immediately; additional scheduled P
     * and direction memory both require the confirmed six-ms shape. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); p.kd=0;
        s.last_error=sign*1.5f;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(sign*s.last_correction>17.99f && sign*s.last_correction<18.01f);
        CHECK(!s.corner_pending_ms && s.mode==CONTROL_TRACK && s.left>0 && s.right>0);
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_pending_ms && s.corner_pending_dir==sign);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.recovery_arc);
        for(i=0;i<13;++i) Control_Step(&s,&p,0,0,0,2);
        CHECK(s.recovery_arc && s.recovery_ms==26);
        Control_Step(&s,&p,0,0,0,2);
        CHECK(!s.recovery_arc && s.recovery_ms==28);
        CHECK(s.left==sign*500 && s.right==-sign*500);
    }
    /* Fresh-corner budget release must use elapsed time and a completed
     * handoff. Center flicker, same-side retries, invalid split retargets,
     * and an opposite cue before sufficient visible travel do not qualify. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        s.stable_ms=0; s.post_turn_ms=200; s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
        for(i=0;i<24/dt;++i) Control_Step(&s,&p,0,1,0x18,dt);
        for(i=0;i<6/dt+1;++i) Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,dt);
        CHECK(!s.post_turn_ms && s.recovery_ms==0 && s.mode==CONTROL_TRACK);
        CHECK(s.corner_pending_dir==-sign && s.last_side==-sign);
        Control_Stop(&s,FAULT_UI);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    Warm(&s,&p);
    s.stable_ms=0; s.post_turn_ms=200; s.post_turn_dir=1; s.recovery_ms=900;
    for(i=0;i<3;++i) Control_Step(&s,&p,-1500,1,0x1F,2);
    CHECK(s.post_turn_ms && s.recovery_ms==906);
    Warm(&s,&p);
    s.stable_ms=0; s.post_turn_ms=200; s.post_turn_dir=1; s.recovery_ms=900;
    for(i=0;i<15;++i) Control_Step(&s,&p,1500,1,0xF8,2);
    CHECK(s.post_turn_ms && s.recovery_ms==930);
    /* Invalid frames interrupt the visible-travel qualification. Even a
     * confirmed new cue after a short glimpse must keep the old deadline. */
    Warm(&s,&p);
    s.stable_ms=0; s.post_turn_ms=200; s.post_turn_dir=1; s.recovery_ms=900;
    for(i=0;i<10;++i) Control_Step(&s,&p,0,1,0x18,2);
    Control_Step(&s,&p,0,0,0,2);
    for(i=0;i<3;++i) Control_Step(&s,&p,-1500,1,0x1F,2);
    CHECK(s.recovery_ms==928 && s.mode==CONTROL_TRACK && s.post_turn_ms);
    /* B29 early-release candidate could shed corner gain on decreasing raw
     * error despite a still-outward filtered rate. B30 retains B28 behavior. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p); p.kd=0;
        s.last_error=sign*2.0f; s.derivative=s.planner_rate=sign*30.0f;
        s.shape_ms=6; s.shape_dir=(int8_t)sign; s.corner_gain=0.6f;
        for(i=1;i<=3;++i) Control_Step(&s,&p,sign*(2000-i*20),1,sign>0?0xF8:0x1F,2);
        CHECK(s.corner_gain>0.55f && !s.corner_releasing);
    }
}
