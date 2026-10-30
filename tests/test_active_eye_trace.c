/* Sampled raw replay only; no interpolation or simulated motor dynamics. */
#include "control.h"
#include "line_estimator.h"
#include "input.h"
volatile int trace[160][12];
volatile unsigned trace_count;
__attribute__((noinline)) void test_done(void) { while(1) {} }
void test_entry(void) {
    LineEstimator line; ControlState s; ControlParameters p; unsigned i;
    Line_Init(&line); line.calibrated=line.white_reference_valid=1;
    line.raw_high_mask=0;
    for(i=0;i<8;++i) { line.low[i]=low[i]; line.high[i]=high[i]; line.white[i]=white[i]; }
    Control_Defaults(&p); Control_Start(&s); s.speed=55;
#ifdef TRACE_ACTIVE_EYE
    s.launch=55;
#endif
    for(i=0;i<160;++i) {
        unsigned dt=i?timestamps[i]-timestamps[i-1]:2;
        Line_Update(&line,raw[i]);
#ifdef TRACE_ACTIVE_EYE
        Control_StepSensors(&s,&p,line.error,line.valid,line.mask,line.strength,dt);
        trace[i][4]=s.active_eye; trace[i][11]=line.position_confidence;
#else
        Control_Step(&s,&p,line.error,line.valid,line.mask,dt);
        trace[i][4]=0; trace[i][11]=line.valid;
#endif
        trace[i][0]=timestamps[i]; trace[i][1]=line.error;
        trace[i][2]=line.valid; trace[i][3]=line.mask;
        trace[i][5]=s.left; trace[i][6]=s.right;
        trace[i][7]=s.mode; trace[i][8]=s.speed_reason;
        trace[i][9]=(int)(s.d_output*1000); trace[i][10]=(int)(s.speed*10);
        ++trace_count;
    }
    test_done();
}
