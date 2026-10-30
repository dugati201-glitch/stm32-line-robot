#ifndef TEST_SWING_EXPECTED_H
#define TEST_SWING_EXPECTED_H
/* Independent numeric expectations for the B43 owner-requested pairs. */
#define SWING_OUTER(maximum,near) ((maximum)<((near)?64:68)?(maximum):((near)?64:68))
#define SWING_INNER(maximum,near) ((maximum)<((near)?22:30)?(maximum):((near)?22:30))
#define SWING_LEFT(sign,maximum,near) ((sign)>0?SWING_OUTER(maximum,near)*10:-(int)SWING_INNER(maximum,near)*10)
#define SWING_RIGHT(sign,maximum,near) ((sign)>0?-(int)SWING_INNER(maximum,near)*10:SWING_OUTER(maximum,near)*10)
#endif
