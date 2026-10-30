#include "control.h"
#include "control_tuning.h"
volatile unsigned test_count,failure_line;
__attribute__((noinline)) void test_done(void) { while(1) {} }
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=220000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static float Abs(float x) { return x<0?-x:x; }
static void Seed(ControlState *s,ControlParameters *p,float kd,int sign)
{
    Control_Defaults(p); p->kp=12; p->ki=0; p->kd=kd;
    p->base_percent=p->straight_percent=55; p->max_percent=85;
    Control_Start(s); s->history=1; s->speed=55; s->left=s->right=550;
    s->post_turn_ms=200; s->post_turn_dir=(int8_t)sign;
    s->last_side=(int8_t)sign; s->last_side_ms=600;
    s->last_error=sign*0.5f; s->fast_exit=1;
}
void test_entry(void)
{
    ControlState s;
    ControlParameters p;
    unsigned i,dt,k;
    int sign;
    const float gains[3]={0.8f,0.9f,1.0f};
    /* Explicit owner profile allows this regression to fail on B36 even
     * before the product default is changed. */
    Seed(&s,&p,0.9f,1);
    Control_Step(&s,&p,-500,1,0x08,2);
    CHECK(Abs(s.last_correction)<12.0f && s.left>0 && s.right>0);
    Seed(&s,&p,0.12f,1);
    Control_Step(&s,&p,-500,1,0x08,2);
    CHECK(Abs(s.last_correction+16.3f)<0.001f); /* 8ms raw D filter, with low-Kd shaping still bypassed. */
    Seed(&s,&p,0.9f,1); s.post_turn_ms=0; s.fast_exit=0; s.last_error=0;
    Control_Step(&s,&p,1000,1,0x30,2);
    CHECK(s.last_correction==42.5f && s.left==850 && s.right==0); /* Outside scoped shaping. */
    for(k=0;k<3;++k) for(dt=1;dt<=4;++dt) for(sign=-1;sign<=1;sign+=2) {
        Seed(&s,&p,gains[k],sign);
        for(i=0;i<64;++i) {
            int e=sign*((i&1U)?500:-500);
            Control_Step(&s,&p,(int16_t)e,1,e>0?0x10:0x08,dt);
            CHECK(Abs(s.last_correction)<=6.301f+2.401f*dt);
            CHECK(s.mode==CONTROL_TRACK && s.target==55 && s.left>0 && s.right>0);
            CHECK(s.left<=850 && s.right<=850);
        }
        /* Sustained D can build, but never above the corner-only cap. */
        s.last_error=sign*0.5f; s.derivative=sign*1000.0f;
        for(i=0;i<10;++i) {
            Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt);
            CHECK(Abs(s.last_correction-sign*6.3f)<=34.001f);
        }
        /* A sign reversal discards old D immediately, then builds the new
         * sign; decreasing demand and Kd=0 must release without delay. */
        s.derivative=-sign*1000.0f;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt);
        CHECK(sign*(s.last_correction-sign*6.3f)<0);
        CHECK(Abs(s.last_correction-sign*6.3f)<=2.401f*dt);
        p.kd=0;
        Control_Step(&s,&p,sign*500,1,sign>0?0x10:0x08,dt);
        CHECK(Abs(s.last_correction-sign*6.3f)<0.001f);
        Control_Stop(&s,FAULT_UI);
        Control_Step(&s,&p,0,1,0x18,dt);
        CHECK(s.mode==CONTROL_FAULT && !s.left && !s.right);
    }
    /* Calm fast exit240pp/s; other eligible exits220pp/s below50%. */
    Seed(&s,&p,0.9f,1); s.speed=43; s.last_error=0;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.speed>43.479f && s.speed<43.481f && s.target==55);
    Seed(&s,&p,0.9f,1); s.speed=43; s.last_error=0; s.derivative=100;
    Control_Step(&s,&p,0,1,0x18,2);
    CHECK(s.speed>43.439f && s.speed<43.441f); /* Raw D is not calm yet. */
    Seed(&s,&p,0.9f,1); s.speed=43; s.last_error=0;
    Control_Step(&s,&p,0,1,0x3C,2);
    CHECK(s.speed>43.439f && s.speed<43.441f); /* Broad patch is not enough. */
    /* Existing direct search, Max and antiwindup remain authoritative. */
    Seed(&s,&p,0.9f,1); p.kp=100; p.ki=2;
    for(i=0;i<8;++i) Control_Step(&s,&p,2000,1,0xF8,2);
    CHECK(s.integral==0 && s.left<=850 && s.right<=850);
    p.max_percent=10; Control_Step(&s,&p,2000,1,0xF8,2);
    CHECK(s.left>=0 && s.right>=0 && s.left<=100 && s.right<=100);
    for(i=0;i<4;++i) Control_Step(&s,&p,0,0,0,2);
    CHECK(s.mode==CONTROL_TURN && s.left==100 && s.right==-100 && !s.recovery_arc);
    Control_Defaults(&p);
    CHECK(p.base_percent==55 && p.straight_percent==55 && p.max_percent==85);
    CHECK(p.kp==12 && p.ki==0 && Abs(p.kd-0.9f)<0.0001f);
    test_done();
}
