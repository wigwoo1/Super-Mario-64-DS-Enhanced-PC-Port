"""Host bindings and tracing for the original particle engine."""


def transform(text,name):
    if name == 'func_0204c304.c':
        # Both divisions call the DS _s32_div_f at 0x01ffabe4.
        # A zero divisor returns the numerator on DS and traps on x86.
        old = '''        p->h3c = 0xffff / (int)((u32)self->h2c >> 1);
        p->h3e = 0xffff / self->h2c;'''
        new = '''        const unsigned int life = self->h2c;
        p->h3c = life >= 2 ? 0xffff / (life >> 1) : 0xffff;
        p->h3e = life ? 0xffff / life : 0xffff;'''
        assert text.count(old) == 1, 'Child particle lifetime divisions changed'
        return 'extern "C" {\n' + text.replace(old, new, 1) + '\n}\n'
    if name=='_ZN10dScStage_c13InitResourcesEv.cpp':
        old='''        (void *)+[](void *) -> int { return 1; },
        (void *)+[](void *) -> int { return 1; },
        (void *)pc_stage_graph_apply,'''
        new='''        (void *)+[](void *) -> int { return 1; },
        (void *)pc_stage_graph_draw_particles,
        (void *)pc_stage_graph_apply,'''
        assert text.count(old)==1,'Stage graphics callback table changed'
        return text.replace(old,new,1)
    if name=='_ZN10dScStage_c14GraphCallback1Ev.cpp':
        # The original callback calls this namespace function through its ARM
        # linker spelling. Use the actual native C++ linkage in the host copy.
        old='extern "C" void _ZN8Particle9RenderAllEv(void);'
        assert text.count(old)==1 and text.count('    _ZN8Particle9RenderAllEv();')==1
        return text.replace(old,'namespace Particle { void RenderAll(); }',1).replace(
            '    _ZN8Particle9RenderAllEv();','    Particle::RenderAll();',1)
    if name=='_ZN8Particle7Manager9AddSystemEiR7Vector3.cpp':
        anchor='    return result;'
        assert text.count(anchor)==1
        text=text.replace(anchor,'    pc_particle_probe::created(this,definitionID,result);\n'+anchor,1)
        return '#include "hal/particle_probe.h"\n'+text
    if name=='_ZN8Particle10SysTracker6UpdateEv.cpp':
        anchor='    func_02049f58(mManager);'
        assert text.count(anchor)==1
        return '#include "hal/particle_probe.h"\n'+text.replace(anchor,anchor+'\n    pc_particle_probe::update(mManager);',1)
    assert name=='func_0204a5c8.c',name
    anchor='void func_0204a5c8(char* self)\n{'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'\n    pc_particle_probe::Scope particleProbe(self);',1)
    # The C++ probe header stays outside the original C linkage block.
    return '#include "hal/particle_probe.h"\nextern "C" {\n'+text+'\n}\n'
