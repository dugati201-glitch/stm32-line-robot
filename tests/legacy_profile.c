#include "legacy_profile.h"
void TestLegacyDefaults(ControlParameters *p)
{
    Control_Defaults(p);
    p->base_percent=p->straight_percent=45;
    p->kd=0.12f;
}
