import pathlib,tempfile,unittest
import hostgen,particle_host
ROOT=pathlib.Path(__file__).resolve().parents[2]


class ParticleHostTests(unittest.TestCase):
    def test_create_preserves_original_body(self):
        name='_ZN8Particle7Manager9AddSystemEiR7Vector3.cpp'
        source=hostgen.transform((ROOT/'src'/name).read_text())[0]
        native=particle_host.transform(source,name)
        restored=native.removeprefix('#include "hal/particle_probe.h"\n').replace(
            '    pc_particle_probe::created(this,definitionID,result);\n','')
        self.assertEqual(source,restored)

    def test_update_preserves_original_body(self):
        name='_ZN8Particle10SysTracker6UpdateEv.cpp'
        source=hostgen.transform((ROOT/'src'/name).read_text())[0]
        native=particle_host.transform(source,name)
        restored=native.removeprefix('#include "hal/particle_probe.h"\n').replace(
            '\n    pc_particle_probe::update(mManager);','')
        self.assertEqual(source,restored)

    def test_render_observes_original_dispatch(self):
        name='func_0204a5c8.c'
        source=hostgen.transform((ROOT/'src/unnamed/arm9/0204'/name).read_text())[0]
        native=particle_host.transform(source,name)
        restored=native.removeprefix('#include "hal/particle_probe.h"\nextern "C" {\n').removesuffix('\n}\n').replace(
            '\n    pc_particle_probe::Scope particleProbe(self);','')
        self.assertEqual(source,restored)

    def test_drift_rejected(self):
        for name in ('_ZN8Particle7Manager9AddSystemEiR7Vector3.cpp',
                     '_ZN8Particle10SysTracker6UpdateEv.cpp','func_0204a5c8.c'):
            with self.assertRaises(AssertionError):particle_host.transform('',name)

    def test_stage_slot1_uses_original_native_worker(self):
        name='_ZN10dScStage_c13InitResourcesEv.cpp'
        source=hostgen.transform((ROOT/'src'/name).read_text())[0]
        native=particle_host.transform(source,name)
        self.assertIn('(void *)pc_stage_graph_draw_particles,\n        (void *)pc_stage_graph_apply,',native)
        self.assertEqual(source.count('return 1;')-1,native.count('return 1;'))
        name='_ZN10dScStage_c14GraphCallback1Ev.cpp'
        native=particle_host.transform((ROOT/'src'/name).read_text(),name)
        self.assertIn('    Particle::RenderAll();',native)
        self.assertNotIn('_ZN8Particle9RenderAllEv',native)

    def test_explicit_renderer_generation_uses_probe(self):
        path=ROOT/'src/unnamed/arm9/0204/func_0204a5c8.c'
        with tempfile.TemporaryDirectory() as directory:
            output,_=hostgen.emit(path,pathlib.Path(directory),ROOT)
            native=output.read_text()
        self.assertIn('pc_particle_probe::Scope particleProbe(self);',native)
        self.assertLess(native.index('#include "hal/particle_probe.h"'),native.index('extern "C" {'))


if __name__=='__main__':unittest.main()
