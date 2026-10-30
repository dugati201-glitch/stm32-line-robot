#include "line_estimator.h"
extern volatile unsigned test_count,failure_line;
void test_done(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=190000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static void Configure(LineEstimator *s,unsigned polarity)
{
    unsigned i;
    Line_Init(s); s->calibrated=s->white_reference_valid=1;
    s->raw_high_mask=polarity?0xFF:0;
    for(i=0;i<8;++i) { s->low[i]=0; s->high[i]=1000; s->white[i]=polarity?0:1000; }
}
static void Update(LineEstimator *s,const uint16_t strength[8],unsigned polarity)
{
    uint16_t raw[8]; unsigned i;
    for(i=0;i<8;++i) raw[i]=polarity?strength[i]:1000-strength[i];
    Line_Update(s,raw);
}
void RunLocalCentroidTests(void)
{
    LineEstimator s;
    uint16_t strength[8];
    unsigned i,polarity,mirror,mask;
    for(polarity=0;polarity<2;++polarity) for(mirror=0;mirror<2;++mirror) {
        Configure(&s,polarity);
        for(i=0;i<8;++i) strength[i]=0;
        strength[3]=strength[4]=1000; strength[mirror?0:7]=400;
        Update(&s,strength,polarity);
        /* An isolated sub-threshold reflection must not pull a centered
         * single black group toward the edge. B28 produces +/-584 here. */
        CHECK(s.valid && s.mask==0x18 && s.error==0);
        CHECK(s.strength[mirror?0:7]==400); /* Preserve diagnostic strengths. */
        /* Keep weak shoulders connected to the group: do not quantize the
         * position to just the binary mask or lose normal sub-eye motion. */
        for(i=0;i<8;++i) strength[i]=0;
        strength[2]=250; strength[3]=750; strength[4]=1000; strength[5]=400;
        if(mirror) for(i=0;i<4;++i) { uint16_t t=strength[i]; strength[i]=strength[7-i]; strength[7-i]=t; }
        Update(&s,strength,polarity);
        CHECK(s.valid && s.mask==0x18 && s.error==(mirror?-146:145));
        /* Do not invent a branch selection from multiple strong groups. */
        strength[0]=strength[7]=1000; s.error=123;
        Update(&s,strength,polarity);
        CHECK(!s.valid && s.error==123 && (s.mask&0x81)==0x81);
    }
    /* All binary masks keep their original validity and raw mask policy. */
    for(polarity=0;polarity<2;++polarity) for(mask=0;mask<256;++mask) {
        unsigned groups=0,previous=0;
        Configure(&s,polarity); s.error=321;
        for(i=0;i<8;++i) {
            unsigned active=(mask>>i)&1U;
            strength[i]=active?1000:0;
            if(active && !previous) ++groups;
            previous=active;
        }
        Update(&s,strength,polarity);
        CHECK(s.mask==mask && s.valid==(mask!=0 && mask!=255 && groups==1));
        if(!s.valid) CHECK(s.error==321);
        else CHECK(s.error>=-3500 && s.error<=3500);
    }
}
