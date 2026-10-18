"""Focused checks for native boss callback and Bob-omb carry adaptations."""
import unittest
from bombking_host import ROOT,inventory,transform


class BossHostTests(unittest.TestCase):
    def test_original_manifest(self):
        states,handles=inventory()
        self.assertEqual(len(states),18)
        self.assertEqual(len(handles),13)
        self.assertEqual(states['data_ov078_02126ffc']['lo'],0x021243c0)
        self.assertEqual(states['data_ov078_0212705c']['hi'],0x021238ac)

    def test_native_dispatch(self):
        source=(ROOT/'src/actors/daBombking_c.cpp').read_text()
        native=transform(source)
        self.assertNotIn('this->**',native)
        self.assertIn('state->enter.call(this)',native)
        self.assertIn('tick.call(this)',native)
        self.assertIn('sizeof(PcBossState)==16',native)
        self.assertIn('mHeldActor->GetHealth()',native)
        self.assertEqual(transform(native),native)

    def test_every_descriptor_bound(self):
        native=transform((ROOT/'src/actors/daBombking_c.cpp').read_text())
        states,handles=inventory()
        for name,pair in states.items():
            self.assertIn(f'reinterpret_cast<PcBossState *>(&{name})',native)
            for target in pair.values():self.assertIn(f'pc_boss_{target:08x}',native)
        for name,file_id in handles.items():
            self.assertIn(f'reinterpret_cast<SharedFilePtr *>(&{name})->Construct({file_id})',native)

    def test_bobomb_contiguous_vectors(self):
        native=transform((ROOT/'src/actors/daBmb_c.cpp').read_text())
        self.assertIn('pc_bmb_carry[12]',native)
        for offset in range(3):self.assertIn(f'(pc_bmb_carry+{offset})',native)
        self.assertNotIn('data_ov102_0214ea18',native)
        self.assertIn('sizeof(daBmb_c)==0x400',native)

    def test_unrelated_unchanged(self):
        self.assertEqual(transform('void unrelated() {}'),'void unrelated() {}')


if __name__=='__main__':unittest.main()
