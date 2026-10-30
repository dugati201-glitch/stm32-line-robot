/* Two synthetic command scenarios; no vehicle or motor model. */
#include "control.h"
volatile unsigned test_count,failure_line,trace_count;
volatile int trace[64][8];
__attribute__((noinline)) void test_done(void) { while(1) {} }
void test_entry(void)
{
    static ControlState s;
    ControlParameters p;
    unsigned i;
    Control_Defaults(&p); p.kp=12; p.ki=0; p.kd=0.9f;
    p.base_percent=p.straight_percent=55; p.max_percent=85;
    Control_Start(&s); s.history=1; s.post_turn_ms=200; s.post_turn_dir=1;
    s.fast_exit=1;
#ifdef TRACE_EXIT
    s.speed=43; s.last_error=0;
#else
    s.speed=55; s.last_error=0.5f;
#endif
    for(i=0;i<64;++i) {
#ifdef TRACE_EXIT
        int error=0;
#else
        int error=(i&1U)?500:-500;
#endif
        Control_Step(&s,&p,(int16_t)error,1,error>0?0x10:error<0?0x08:0x18,2);
        trace[i][0]=(i+1)*2; trace[i][1]=error;
        trace[i][2]=(int)(s.last_correction*1000.0f);
        trace[i][3]=s.left; trace[i][4]=s.right;
        trace[i][5]=(int)(s.target*1000.0f); trace[i][6]=s.mode;
        trace[i][7]=(int)(s.speed*1000.0f);
    }
    trace_count=64; test_done();
}
