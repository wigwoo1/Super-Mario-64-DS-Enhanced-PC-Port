//cpp
// @symbol _ZN5dPa_c7level_c15scaleCallback_c14SpawnParticlesERN8Particle6SystemE
#include "dPa_c.h"
#include "Particle__System.h"
#if defined(SM64DS_PLATFORM_PC)
extern "C" void func_02049d60(Particle::Manager *, Particle::System *);
#endif

void dPa_c::level_c::scaleCallback_c::SpawnParticles(Particle::System& system)
{
    system.callbackScale = scale;
    system.callbackVelocity = velocity;
#if defined(SM64DS_PLATFORM_PC)
    system.callbackFlags |= 2;
    system.callbackValue = value;
    func_02049d60(data_0209ee74->mManager, &system);
#else
    simpleCallback_c::SpawnParticles(system);
#endif
    timer = 0x10;
}
