//cpp
/* dBase_c -- the middle link of the actor hierarchy: fBase_c -> dBase_c ->
 * dActor_c (arm9 0x02013e80..0x02013f28).
 *
 * Adds no members and no new virtuals; AfterInitResources (slot 2) is the key
 * function, so this TU owns the vtable with D1/D0 dragged in beside it and no
 * forcing scaffold. Spawn veneers the actor factory; func_02013edc veneers the
 * scene-spawn path. Written last-to-first: mwccarm emits .text in reverse
 * source order. Size is asserted in include/dBase_c.h (0x50, fBase_c's own).
 *
 * deslop leftovers:
 * - func_02013edc keeps its ROM label: the scene-creation veneer, enclosed by
 *   this class's own run. It returns the scene factory result to
 *   dScene_c::SpawnIfNecessary, which checks r0 before updating scene state.
 * - Spawn's trailing (int, int) stay unnamed: dActor_c::Spawn passes
 *   (spawnParam, 2) and other callers (mode/entranceId, 0), and both words
 *   ride r2/r3 through this veneer into func_02042ffc's factory call, but
 *   their roles belong to the factory's TU (func_02043098), not this one.
 * - func_02042ffc / func_02042fe4 keep their ROM labels for the same reason.
 */
#include "dBase_c.h"

extern "C" {
extern int func_02042fe4(int a, int b, int c);
extern fBase_c *func_02042ffc(u32 actorID, fBase_c *parent, int spawnParam, int actorType);
}

// @symbol _ZN7dBase_c18AfterInitResourcesEj
void dBase_c::AfterInitResources(u32 vfSuccess)
{
    if (vfSuccess == 1)
        MarkForDestruction();
    fBase_c::AfterInitResources(vfSuccess);
}

// @symbol _ZN7dBase_c5SpawnEjP7fBase_cii
fBase_c *dBase_c::Spawn(u32 actorID, fBase_c *parent, int spawnParam, int actorType)
{
    return func_02042ffc(actorID, parent, spawnParam, actorType);
}

// @symbol func_02013edc
extern "C" int func_02013edc(int a, int b, int c)
{
    return func_02042fe4(a, b, c);
}
