import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'port/tools'))
import chomp_host
import hostgen


class ChompHostTest(unittest.TestCase):
    def transformed(self, relative):
        path = ROOT/relative
        return chomp_host.transform(ROOT, hostgen.transform(path.read_text())[0], path.name)

    def test_profiles_from_original_image(self):
        self.assertEqual(chomp_host.inventory(ROOT, 'd_a_wanwan.cpp'),
                         (219,67,0x10000003,0x120000,0x400000,0x1000000,0x1000000))
        self.assertEqual(chomp_host.inventory(ROOT, 'daObjPile_c.cpp')[:3], (27,295,0x08000002))
        self.assertEqual(chomp_host.inventory(ROOT, 'd_a_obj_wanwan_shutter.cpp')[:3], (41,68,2))

    def test_native_factory_callbacks_and_resource_ids(self):
        result = self.transformed('src/game/actors/d_a_wanwan.cpp')
        self.assertIn('return new daWanwan_c();', result)
        self.assertNotIn('*(int **)c = &_ZTV', result)
        self.assertNotIn('(c->*data_', result)
        self.assertEqual(result.count('extern "C" WanwanPmf data_ov014_'), 24)  # declaration and native definition
        for file_id in ('0x9c01','0x9c02','0x9c03','0x9c04'):
            self.assertEqual(result.count(f'.Construct({file_id})'), 1)
        self.assertNotIn('pc_sharedfile_handle_construct(this, fileID)', result)
        self.assertIn('sizeof(WanwanStateRow)==20', result)

    def test_stump_and_fence_keep_original_gameplay(self):
        stump = self.transformed('src/actors/daObjPile_c.cpp')
        fence = self.transformed('src/game/actors/d_a_obj_wanwan_shutter.cpp')
        self.assertIn('mStepsLeft = 3;', stump)
        self.assertIn('void daObjPile_c::OnGroundPounded(dActor_c &other)', stump)
        self.assertIn('*q = *q - 1;', stump)
        self.assertIn('a->mDisabled = 1;', fence)
        self.assertIn('a->mMeshCollider.Disable();', fence)
        for result in (stump, fence):
            self.assertNotIn('pc_sharedfile_handle_construct(this, fileID)', result)

    def test_unrelated_file_unchanged(self):
        self.assertEqual(chomp_host.transform(ROOT, 'int f();', 'other.cpp'), 'int f();')

    def test_first_distant_draw_restores_flags_and_respects_area(self):
        result = self.transformed('src/actors/dActor_c.cpp')
        self.assertIn('(actorID==219 || actorID==27 || actorID==41)', result)
        self.assertIn('(mAreaId<0 || IsAreaShowing(mAreaId))', result)
        self.assertIn('mCamSpacePosZ!=0x7fffffff', result)
        self.assertIn('if(mClipResult&0x10)mFlags|=0x20;', result)
        # Other actors may deliberately set their hide bit during Render.
        after=result.split('void dActor_c::AfterRender(u32 vfSuccess)',1)[1].split('// @symbol',1)[0]
        self.assertNotIn('mFlags&=~0x38', after)
        original=(ROOT/'src/actors/dActor_c.cpp').read_text()
        def behavior(text):
            return text.split('int dActor_c::BeforeBehavior()',1)[1].split('// @symbol _ZN8dActor_c13AfterBehaviorEj',1)[0]
        self.assertEqual(behavior(result), behavior(hostgen.transform(original)[0]))


if __name__ == '__main__':
    unittest.main()
