#include "control.h"
#include "control_tuning.h"
void B40_Control_Defaults(ControlParameters *p);
void B40_Control_Start(ControlState *s);
void B40_Control_Step(ControlState *s,const ControlParameters *p,int16_t e,uint8_t valid,uint8_t mask,uint32_t dt);
volatile unsigned test_count,failure_line;
/* Capture the actual ARM commands for the documented input sequence. */
volatile int b41_trace[6][7];
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line) { ++test_count; if(!ok) { failure_line=260000+line; test_done(); } }
#define CHECK(x) Check((x),__LINE__)
static void Warm(ControlState *s,ControlParameters *p,unsigned base,int old) {
    unsigned i; Control_Defaults(p); p->base_percent=p->straight_percent=base;
    if(old) B40_Control_Start(s); else Control_Start(s);
    s->speed=(float)base;
    for(i=0;i<60;++i) {
        if(old) B40_Control_Step(s,p,88,1,0x18,2);
        else Control_Step(s,p,88,1,0x18,2);
    }
}
void test_entry(void) {
    ControlState s,old; ControlParameters p,q; unsigned i,dt,b,elapsed; int sign;
    const int errors[6]={1506,1565,1624,1635,1646,1637};
    /* 32540/44/48 are logged. Odd-index 2ms samples are interpolated, not measured. */
    for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55,0); Warm(&old,&q,55,1);
        Control_Step(&s,&p,88,0,sign>0?0x98:0x19,2);
        B40_Control_Step(&old,&q,88,0,sign>0?0x98:0x19,2);
        for(i=0;i<6;++i) {
            Control_Step(&s,&p,sign*errors[i],1,sign>0?0xF8:0x1F,2);
            B40_Control_Step(&old,&q,sign*errors[i],1,sign>0?0xF8:0x1F,2);
            CHECK(s.mode==CONTROL_TRACK && s.left+s.right>0 && !s.brake_count);
            CHECK(s.left>=-240 && s.right>=-240 && s.left<=850 && s.right<=850);
            if(i>=1) {
                CHECK(sign>0?s.right<0:s.left<0);
                CHECK(s.left+s.right<old.left+old.right);
            }
            if(sign>0) {
                b41_trace[i][0]=32540+2*i; b41_trace[i][1]=errors[i];
                b41_trace[i][2]=old.left; b41_trace[i][3]=old.right;
                b41_trace[i][4]=s.left; b41_trace[i][5]=s.right;
                b41_trace[i][6]=(int)(s.last_correction*1000);
            }
        }
        Control_Step(&s,&p,0,1,0x18,2);
        CHECK(s.mode==CONTROL_TRACK && s.left>=0 && s.right>=0);
    }
    /* Logged 4ms sampling only: no invented intermediate error samples. */
    for(sign=-1;sign<=1;sign+=2) {
        const int logged[3]={1506,1624,1646};
        Warm(&s,&p,55,0); Warm(&old,&q,55,1);
        Control_Step(&s,&p,88,0,sign>0?0x98:0x19,2);
        B40_Control_Step(&old,&q,88,0,sign>0?0x98:0x19,2);
        for(i=0;i<3;++i) {
            Control_Step(&s,&p,sign*logged[i],1,sign>0?0xF8:0x1F,4);
            B40_Control_Step(&old,&q,sign*logged[i],1,sign>0?0xF8:0x1F,4);
            CHECK(s.mode==CONTROL_TRACK && s.left+s.right>0);
            CHECK(sign>0?s.right<0:s.left<0);
            CHECK(s.left+s.right<old.left+old.right);
        }
    }
    for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Warm(&s,&p,55,0); p.kd=0; s.last_error=sign*1.5f;
        elapsed=0;
        do {
            Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,dt); elapsed+=dt;
            CHECK((s.drive_corner_dir!=0)==(elapsed>=4));
            if(elapsed>=2) CHECK(sign*s.last_correction>18);
        } while(elapsed<4);
        CHECK(s.drive_corner_dir==sign && s.corner_pending_dir==sign);
        p.kp=100; p.ki=2;
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,sign*1646,1,sign>0?0xF8:0x1F,dt);
            CHECK(s.integral==0 && s.left+s.right>0);
            CHECK(s.left>=-240 && s.right>=-240 && s.left<=850 && s.right<=850);
        }
        Control_Step(&s,&p,sign*1300,1,sign>0?0xF8:0x1F,dt);
        CHECK(s.left>=0 && s.right>=0); /* Clear return releases reverse. */
        Control_Step(&s,&p,0,0,0,dt);
        CHECK(s.mode==CONTROL_TURN && s.turn_dir==sign && !s.recovery_arc);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_ALIGN && s.target==52 && s.speed>=48 && s.speed<=49);
        Control_Stop(&s,FAULT_UI); Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    Warm(&s,&p,55,0); p.kp=22; p.ki=2; p.kd=0;
    s.last_error=1.65f; s.corner_pending_ms=200; s.drive_corner_dir=s.corner_pending_dir=1;
    s.shape_dir=1; s.shape_ms=4; s.corner_gain=0;
    Control_Step(&s,&p,1650,1,0xF8,2);
    CHECK(s.right==-240 && s.integral==0); /* Wheel clipping, below correction limit. */
    /* No turn-side confirmation from alternating single samples or a center patch. */
    Warm(&s,&p,55,0);
    for(i=0;i<50;++i) {
        sign=(i&1)?1:-1;
        Control_Step(&s,&p,sign*1500,1,sign>0?0xF8:0x1F,2);
        CHECK(!s.drive_corner_dir && !s.corner_pending_ms && s.left>=0 && s.right>=0);
    }
    /* Stronger cap still obeys Max for low duty / zero gains / startup. */
    for(b=0;b<5;++b) {
        const unsigned bases[5]={10,20,30,45,55};
        Warm(&s,&p,bases[b],0); p.kp=p.ki=p.kd=0;
        for(i=0;i<20;++i) {
            Control_Step(&s,&p,1646,1,0xF8,2);
            CHECK(s.left==s.right && s.last_correction==0 && s.left>0);
            CHECK(s.left<=bases[b]*10);
        }
    }
    /* White dashes: unchanged exact held pair and bounded timeout. */
    Warm(&s,&p,55,0);
    for(i=0;i<100;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<100;++i) {
        Control_Step(&s,&p,88,0,0,2);
        CHECK(s.mode==CONTROL_GAP && s.left==550 && s.right==550);
    }
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.mode==CONTROL_TRACK);
    test_done();
}
