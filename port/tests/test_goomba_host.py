import pathlib
import sys
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
import goomba_host


class GoombaHostTest(unittest.TestCase):
    def test_native_callbacks_and_handles(self):
        source=(ROOT/'src/actors/daKrb_c.cpp').read_text()
        result=goomba_host.transform(source)
        self.assertIn('data_ov084_02130d74[st].fn(this);',result)
        self.assertIn('sizeof(StateHandler)==8',result)
        self.assertIn('param1=pc_test_conditional_goomba_param(param1, data_0209f2f8);',result)
        self.assertIn('{pc_goomba_airborne, 0}',result)
        self.assertEqual(result.count('_ZN13SharedFilePtr9ConstructEj(&data_ov084_'),9)
        self.assertEqual(goomba_host.transform(result),result)

    def test_unrelated_source_unchanged(self):
        self.assertEqual(goomba_host.transform('int f(){return 1;}'),'int f(){return 1;}')

    def test_actor_probe_preserves_include_order_and_is_idempotent(self):
        # These actors deliberately need common.h's flat matrix definition.
        # Reinstrumentation used to create duplicate local declarations.
        for actor in ('daCoin_c','daBmb_c'):
            source=(ROOT/f'src/actors/{actor}.cpp').read_text()
            result=goomba_host.transform(source)
            self.assertEqual(result.count('pc_render_probe::Scope renderProbe'),1)
            self.assertLess(result.index('#include "common.h"'),
                            result.index('#include "hal/actor_render_probe.h"'))
            self.assertEqual(goomba_host.transform(result),result)

    def test_collision_veneer_forwards_receiver(self):
        for rel in ('src/unnamed/arm9/0203/func_02038414.c',
                    'src/dBgCh_Actr_UpdateDiscreteNoLava_veneer.c'):
            text=goomba_host.transform((ROOT/rel).read_text())
            self.assertIn('(void *self)',text)
            self.assertIn('Ev(self);',text)
            self.assertEqual(goomba_host.transform(text),text)

    def test_yoshi_egg_uses_native_virtual_method(self):
        text=goomba_host.transform((ROOT/'src/_ZN6Player18St_YoshiPower_MainEv.cpp').read_text())
        self.assertIn('->OnTurnIntoEgg(*this);',text)
        self.assertNotIn('->v19(c);',text)
        self.assertNotIn('->v18()',text)
        self.assertIn('using Obj = dActor_c;',text)

    def test_bite_and_crouch_use_native_queries(self):
        for suffix in ('6998','6790'):
            text=goomba_host.transform((ROOT/f'src/unnamed/ov002/func_ov002_020d{suffix}.cpp').read_text())
            self.assertIn('pc_actor_yoshi_try_eat(',text)
            self.assertNotIn('->V18()',text)
            self.assertNotIn('->v18()',text)
            self.assertNotIn('->v19(self)',text)


if __name__=='__main__':unittest.main()
