#include "control.h"
#include "control_tuning.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=280000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static void Seed(ControlState *s,ControlParameters *p,int sign,float speed,unsigned maximum) {
    Control_Defaults(p); p->max_percent=maximum;
    Control_Start(s); s->history=1; s->mode=CONTROL_TURN; s->turn_dir=(int8_t)sign;
    s->speed=speed;
}
void test_entry(void) {
    ControlState s; ControlParameters p; unsigned dt,i,j,k,ms; int sign;
    const unsigned maxima[7]={10,20,30,40,55,85,100};
    Control_Defaults(&p);
    CHECK(p.base_percent==55 && p.max_percent==85 && p.kp==12 && p.ki==0 && p.kd==0.9f);
    Seed(&s,&p,1,55,85); s.mode=CONTROL_TRACK;
    Control_Step(&s,&p,500,1,0x10,2);
    CHECK(s.derivative>41.66f && s.derivative<41.67f); /* Measured-step test of10ms filter. */
    s.last_error=0.5f; s.derivative=1000;
    for(i=0;i<10;++i) Control_Step(&s,&p,500,1,0x10,2);
    CHECK(s.d_output==28.0f);
    Seed(&s,&p,1,55,85); s.mode=CONTROL_TRACK; p.kp=100; p.ki=2;
    for(i=0;i<8;++i) Control_Step(&s,&p,1650,1,0xF8,2);
    CHECK(s.left==850 && s.right==-240 && s.integral==0);
    for(sign=-1;sign<=1;sign+=2) for(dt=1;dt<=4;++dt) {
        Seed(&s,&p,sign,55,85);
        Control_Step(&s,&p,0,0,0xB8,dt);
        CHECK(s.mode==CONTROL_TURN && s.fault==FAULT_NONE);
        CHECK(s.left==(sign>0?640:-220) && s.right==(sign>0?-220:640));
        CHECK(s.target==64 && s.speed==55 && s.left+s.right==420);
        CHECK(s.recovery_ms==dt && !s.brake_count && !s.recovery_arc);
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.left==(sign>0?680:-300) && s.right==(sign>0?-300:680));
        CHECK(s.target==68 && s.speed==55 && s.left+s.right==380);
        CHECK(s.recovery_ms==2*dt);
        for(i=0;i<15;++i) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.speed==55 && s.recovery_ms==17*dt);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.target==55 && s.speed==55);
        CHECK(s.left>0 && s.right>0); /* Valid line cancels swing immediately. */
        /* Duty carry must not boost a low-duty approach on rejoin. */
        Seed(&s,&p,sign,5,85);
        Control_Step(&s,&p,0,0,0,dt); CHECK(s.speed==5);
        Control_Step(&s,&p,0,1,0x18,dt); CHECK(s.speed>5 && s.speed<=5+0.151f*dt);
        for(j=0;j<7;++j) for(k=0;k<2;++k) {
            unsigned maximum=maxima[j],outer=k?64:68,inner=k?22:30;
            int l,r;
            if(outer>maximum) outer=maximum;
            if(inner>maximum) inner=maximum;
            l=sign>0?(int)outer*10:-(int)inner*10;
            r=sign>0?-(int)inner*10:(int)outer*10;
            Seed(&s,&p,sign,80,maximum);
            Control_Step(&s,&p,0,0,k?0xB8:0,dt);
            CHECK(s.left==l && s.right==r && s.target==outer);
            CHECK(s.speed<=(maximum<55?maximum:55));
            CHECK(s.left+s.right>=0 && s.left<=(int)maximum*10 && s.right<=(int)maximum*10);
            CHECK(s.left>=-(int)maximum*10 && s.right>=-(int)maximum*10);
        }
    }
    /* Short chicane: continuous valid travel totals8ms including fresh shape. */
    for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,sign,55,85); s.mode=CONTROL_TRACK; s.post_turn_ms=200;
        s.post_turn_dir=(int8_t)sign; s.recovery_ms=900;
        for(i=0;i<8;++i) Control_Step(&s,&p,0,1,0x18,1);
        for(i=0;i<3;++i) Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,1);
        CHECK(s.post_turn_ms && s.recovery_ms==911);
        Control_Step(&s,&p,-sign*1500,1,sign>0?0x1F:0xF8,1);
        CHECK(!s.post_turn_ms && !s.recovery_ms && s.corner_pending_dir==-sign);
        Control_Step(&s,&p,0,0,0,1);
        CHECK(s.turn_dir==-sign && s.left==(-sign>0?680:-300) && s.right==(-sign>0?-300:680));
    }
    /* Post-turn center clear is exactly40ms, while mere glimpses cannot renew timeout. */
    Seed(&s,&p,1,48,85); s.mode=CONTROL_TRACK; s.post_turn_ms=200; s.post_turn_dir=1; s.recovery_ms=100;
    for(i=0;i<15;++i) Control_Step(&s,&p,0,1,0x18,1);
    CHECK(s.post_turn_ms && s.recovery_ms==115);
    Control_Step(&s,&p,0,1,0x18,1); CHECK(s.post_turn_ms && !s.recovery_ms);
    for(i=0;i<24;++i) Control_Step(&s,&p,0,1,0x18,1); CHECK(!s.post_turn_ms && !s.recovery_ms);
    Seed(&s,&p,1,55,85); s.recovery_ms=1198;
    Control_Step(&s,&p,1500,1,0x20,2); CHECK(s.mode==CONTROL_ALIGN && s.fault==FAULT_NONE);
    Control_Step(&s,&p,0,0,0,2); CHECK(s.mode==CONTROL_FAULT && s.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
    for(dt=1;dt<=4;++dt) {
        Seed(&s,&p,1,55,85);
        for(ms=0;ms<1200;ms+=dt) Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    /* Twelve distinct corners complete handoff and must not accumulate false timeout. */
    Seed(&s,&p,1,55,85);
    for(i=0;i<12;++i) {
        sign=(i&1)?1:-1;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        Control_Step(&s,&p,0,0,0,2);
        for(j=0;j<24;++j) Control_Step(&s,&p,sign*1000,1,sign>0?0x30:0x0C,2);
        CHECK(s.mode==CONTROL_TRACK && s.fault==FAULT_NONE && !s.recovery_ms);
    }
    /* Existing forward dash memory and explicit fault latch stay active. */
    Control_Defaults(&p); Control_Start(&s);
    for(i=0;i<400;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<140;++i) { Control_Step(&s,&p,0,0,0,2); CHECK(s.mode==CONTROL_GAP && s.left==550 && s.right==550); }
    Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,1,0x18,2); CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    test_done();
}
