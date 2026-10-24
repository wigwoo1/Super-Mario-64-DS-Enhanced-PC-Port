import pathlib
import unittest
import hostgen
import battlefield_platform_host as adapter

ROOT=pathlib.Path(__file__).resolve().parents[2]


class BattlefieldPlatformHostTests(unittest.TestCase):
    def native(self,path):
        original=(ROOT/path).read_text()
        return original,adapter.transform(hostgen.transform(original)[0],pathlib.Path(path).name)

    def test_seesaw_original_contact_and_spring_logic(self):
        original,native=self.native('src/actors/daObjSeesaw_c.cpp')
        for name in ('func_ov095_0213579c','func_ov095_021358cc','func_ov095_02135cdc','func_ov095_02135e90'):
            start=original.index('// @symbol '+name)
            end=original.find('// @symbol',start+12)
            self.assertIn(original[start:end].rstrip(),native)
        self.assertIn('if (offScreen != 0)',native)
        self.assertIn('if (mMeshCollider.IsEnabled())\n            mMeshCollider.Disable();',native)
        self.assertNotIn('data_ov095_021374a0 +',native)
        self.assertIn('data_ov095_02137780.Construct(1445)',native)
        self.assertIn('assert(actorID==ACTOR_BOMB_SEESAW && mVariant==1)',native)

    def test_lift_native_member_offsets_and_original_motion(self):
        original,native=self.native('src/actors/daObjRotateUpdownLift_c.cpp')
        for expression in ('data_ov091_02134cdc[mVariant].node[5].y',
                           'this, &mShadowModel, pad_348,','*reinterpret_cast<s16 *>(&mPitchBase)',
                           'mModel.mat4x3.t.y = mPosY >> 3;',
                           'Spawn(0x1d, 0xffff, spawnAt, 0, mAreaId, -1);',
                           'mWaypointIndex = (u8)(param1 & 0xf);',
                           'if (dist == 0 || dist <= stepLen)',
                           'if ((u32)mWaypointIndex > 3 && (u32)mWaypointIndex < 8)'):
            self.assertIn(expression,native)
        for raw in ('c + 0x348','c + 0x370','this + 0x3a2','stepAt + 0xa4',
                    'data_ov091_02134c30[','data_ov091_02134d1c['):
            self.assertNotIn(raw,native)

    def test_gate_native_timer_and_original_event_rotation(self):
        _,native=self.native('src/actors/daObjSwdoor_c.cpp')
        self.assertIn('DecIfAbove0_Byte(&static_cast<daObjBSwdoor_c *>(self)->mTimer)',native)
        self.assertIn('_ZN5Event6GetBitEj(static_cast<daObjBSwdoor_c *>(self)->mEventBit)',native)
        for raw in ('c+0x31','c+0x320','c+0x321','self + 0x31','self + 0x320','self + 0x321'):
            self.assertNotIn(raw,native)
        self.assertIn('*LMS(&self->mAngleY) += 0x100;',native)
        self.assertIn('*LMS(&self->mAngleY) -= 0x100;',native)
        _,leaf=self.native('src/game/actors/d_a_obj_b_swdoor.cpp')
        self.assertIn('pc_battlefield_gate_init_resources();',leaf)
        self.assertIn('reinterpret_cast<CLPS_Block *>(data_ov014_02114578)',leaf)

    def test_handles_are_lazy_and_drift_rejected(self):
        for path in ('src/actors/daObjSeesaw_c.cpp','src/actors/daObjRotateUpdownLift_c.cpp',
                     'src/game/actors/d_a_obj_b_swdoor.cpp'):
            _,native=self.native(path)
            self.assertNotIn('pc_sharedfile_handle_construct(this,',native)
            with self.assertRaises(AssertionError):
                adapter.transform('',pathlib.Path(path).name)


if __name__=='__main__':
    unittest.main()
