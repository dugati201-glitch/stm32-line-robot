#ifndef TEST_SWING_EXPECTED_H
#define TEST_SWING_EXPECTED_H
/* Independent numeric expectations for the B46 owner-requested pairs. */
#define SWING_OUTER(maximum,near) ((maximum)<((near)?66:70)?(maximum):((near)?66:70))
#define SWING_INNER(maximum,near) ((maximum)<((near)?24:33)?(maximum):((near)?24:33))
#define SWING_LEFT(sign,maximum,near) ((sign)>0?SWING_OUTER(maximum,near)*10:-(int)SWING_INNER(maximum,near)*10)
#define SWING_RIGHT(sign,maximum,near) ((sign)>0?-(int)SWING_INNER(maximum,near)*10:SWING_OUTER(maximum,near)*10)
#endif
