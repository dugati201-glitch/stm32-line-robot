/* Same inputs on archived B38/current B39; commands only, no vehicle model. */
#include "control.h"
#ifndef TRACE_BASE
#define TRACE_BASE 55
#endif
#ifndef TRACE_SCENARIO
#define TRACE_SCENARIO 0
#endif
volatile unsigned trace_count;
volatile int trace[64][8];
__attribute__((noinline)) void test_done(void) { while(1) {} }
void test_entry(void) {
    ControlState s; ControlParameters p; unsigned i;
    Control_Defaults(&p); p.base_percent=p.straight_percent=TRACE_BASE;
    Control_Start(&s); s.speed=TRACE_BASE;
    for(i=0;i<60;++i) Control_Step(&s,&p,0,1,0x18,2);
    for(i=0;i<64;++i) {
        int e=0; unsigned mask=0x18,valid=1;
#if TRACE_SCENARIO == 0
        if(i<3) { e=1500; mask=0xF8; }
        else if(i<19) { e=2500; mask=0x40; }
        else if(i<25) { e=3500; mask=0x80; }
        else if(i<32) { e=1500; mask=0x20; }
#elif TRACE_SCENARIO == 1
        e=(i<32?i:63-i)*40; mask=0x30;
#elif TRACE_SCENARIO == 2
        e=2500; mask=0x40;
#else
        if(i>=16 && i<26) { valid=0; mask=0; }
#endif
        Control_Step(&s,&p,(int16_t)e,(uint8_t)valid,(uint8_t)mask,2);
        trace[i][0]=(i+1)*2; trace[i][1]=e;
        trace[i][2]=s.left; trace[i][3]=s.right; trace[i][4]=s.mode;
        trace[i][5]=(int)(s.last_correction*1000.0f);
        trace[i][6]=(int)(s.corner_gain*1000.0f); trace[i][7]=s.recovery_ms;
    }
    trace_count=64; test_done();
}
