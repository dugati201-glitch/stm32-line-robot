#ifndef TEST_LEGACY_PROFILE_H
#define TEST_LEGACY_PROFILE_H
#include "control.h"
/* Pin historical 45/85, Kd0.12 regressions independently of product defaults.
 * Current high-Kd profile is tested separately and by menu/log metadata. */
void TestLegacyDefaults(ControlParameters *p);
#endif
