#include "swing_expected.h"
#include "legacy_profile.h"
#include "control.h"
#include "control_tuning.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=200000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p)
{
    unsigned i;
    TestLegacyDefaults(p); Control_Start(s); s->speed=45;
    for(i=0;i<60;++i) Control_Step(s,p,0,1,0x18,2);
}
void RunImmediateCornerTests(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt;
    int sign;
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p);
        for(i=0;i<3;++i) Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(s.mode==CONTROL_TRACK && s.corner_pending_ms && s.left>=-280 && s.right>=-280 && s.left+s.right>0);
        /* No initial forward arc: same signed request as a chained retry,
         * from the first invalid frame following a confirmed visible cue. */
        Control_Step(&s,&p,0,0,0,2);
        CHECK(s.mode==CONTROL_TURN && s.left==SWING_LEFT(sign,p.max_percent,0) && s.right==SWING_RIGHT(sign,p.max_percent,0));
        CHECK(!s.recovery_arc && s.recovery_arc_speed==0 && !s.brake_ms && !s.brake_count);
        Control_Step(&s,&p,0,0,0xFF,2);
        CHECK(s.left==SWING_LEFT(sign,p.max_percent,1) && s.right==SWING_RIGHT(sign,p.max_percent,1) && s.recovery_ms==2);
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_ALIGN && s.left>0 && s.right>0 && s.target==45);
        for(i=0;i<5;++i) Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.post_turn_ms);
        {
            unsigned before=s.recovery_ms;
            for(i=0;i<3;++i) Control_Step(&s,&p,0,0,0,2);
            CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign && s.recovery_ms==before+6);
            CHECK(!s.recovery_arc && s.left==SWING_LEFT(sign,p.max_percent,0) && s.right==SWING_RIGHT(sign,p.max_percent,0));
        }
    }
    /* Invalid split confirmation and generic remembered-side recovery also
     * have no extra forward phase. Preserve confirmation and output limits. */
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        unsigned elapsed=0;
        Warm(&s,&p);
        do {
            Control_Step(&s,&p,0,0,sign>0?0xE8:0x17,dt); elapsed+=dt;
            if(elapsed<6) CHECK(s.mode!=CONTROL_TURN);
        } while(elapsed<6);
        CHECK(s.mode==CONTROL_TURN && s.left==SWING_LEFT(sign,p.max_percent,1) && s.right==SWING_RIGHT(sign,p.max_percent,1));
        CHECK(!s.recovery_arc && !s.recovery_arc_speed);
        p.max_percent=20;
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left==sign*200 && s.right==-sign*200);
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.left>=0 && s.right>=0);
        for(i=0;i<1300/dt;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
        Warm(&s,&p); s.history=1; s.last_error=sign*2.0f;
        s.guide_valid=s.curve_valid=0;
        elapsed=0;
        do { Control_Step(&s,&p,0,0,0,dt); elapsed+=dt; } while(elapsed<6);
        CHECK(s.mode==CONTROL_TURN && s.left==SWING_LEFT(sign,p.max_percent,0) && s.right==SWING_RIGHT(sign,p.max_percent,0));
        CHECK(!s.recovery_arc);
        Control_Stop(&s,FAULT_UI);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
}
