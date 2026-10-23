from pathlib import Path
import struct,sys,tempfile,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import basement_data as data,basement_host,hostgen
class BasementDataTest(unittest.TestCase):
    def fixture(self,root):
        b=bytearray(0x1400);h=data.HEADER-data.BASE
        struct.pack_into('<H',b,h,91)
        struct.pack_into('<H',b,h+16,1)
        for offset in (4,8,12):struct.pack_into('<I',b,h+offset,data.START)
        struct.pack_into('<I',b,h+20,data.START+16)
        struct.pack_into('<I',b,data.START-data.BASE+20,data.START+64)
        for address in (0x02111c90,0x02111cd0):b[address-data.BASE:address-data.BASE+4]=b'CLPS'
        p=root/'extracted/overlays/overlay_0012.bin';p.parent.mkdir(parents=True);p.write_bytes(b);return p,b
    def test_complete_relocation_and_private_material_outputs(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);self.fixture(r);out=r/'out.cpp';offsets=data.generate(r,out)
            self.assertEqual(len(offsets),5)
            self.assertIn('static_assert(sizeof(void *)==4',out.read_text())
            self.assertIn('data_ov012_02111cd0',out.read_text())
    def test_external_animation_pointer_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);p,b=self.fixture(r);struct.pack_into('<I',b,data.HEADER-data.BASE+4,0x02000000);p.write_bytes(b)
            with self.assertRaisesRegex(ValueError,'outside reviewed'):data.generate(r,r/'out.cpp')
    def test_missing_collision_bytes_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d);p,b=self.fixture(r);p.write_bytes(b[:data.END-data.BASE])
            with self.assertRaisesRegex(ValueError,'collision material'):data.generate(r,r/'out.cpp')
    def test_original_actor_sources_defer_handles_until_resources(self):
        root=Path(__file__).resolve().parents[2]
        for water,file in [(False,'d_a_obj_c0_switch.cpp'),(True,'d_a_obj_c0_water.cpp')]:
            s=(root/'src/game/actors'/file).read_text();generated,_=hostgen.transform(s)
            adapted=basement_host.transform(generated,water)
            self.assertNotIn('pc_sharedfile_handle_construct(this',adapted)
            self.assertIn('.Construct(1451)' if water else '.Construct(1449)',adapted)
            self.assertIn('data_0209caa0[2] & 0x80000',adapted)
            if water:self.assertIn('pc_basement_animation()',adapted)
if __name__=='__main__':unittest.main()
