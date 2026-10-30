/* Synthetic requested PWM only: no vehicle or motor dynamics. */
#include "control.h"
volatile unsigned test_count,failure_line,trace_count;
volatile int trace[64][8];
__attribute__((noinline)) void test_done(void) { while(1) {} }
void test_entry(void)
{
    static ControlState s;
    ControlParameters p;
    unsigned i;
    Control_Defaults(&p);
#ifndef TRACE_DEFAULT_PID
    p.ki=p.kd=0;
#endif
    Control_Start(&s); s.speed=45;
    for(i=0;i<64;++i) {
        int e=i<8?0:i<16?1200:i<20?1500:i<26?3500:i<44?3500:i<48?1500:0;
        unsigned mask=i<8?0x18:i<16?0x30:i<20?0xF8:i<26?0x80:i<44?0:i<48?0x60:0x18;
        Control_Step(&s,&p,(int16_t)e,(uint8_t)(i<26 || i>=44),(uint8_t)mask,2);
        trace[i][0]=(i+1)*2; trace[i][1]=e;
        trace[i][2]=(int)(s.last_correction*1000.0f);
        trace[i][3]=s.left; trace[i][4]=s.right;
        trace[i][5]=(int)(s.target*1000.0f);
        trace[i][6]=s.mode; trace[i][7]=s.recovery_arc;
    }
    trace_count=64; test_done();
}
