"""Check rabbit callback adaptation against the local cartridge metadata."""
import pathlib
import unittest
import hostgen
import rabbit_host

ROOT=pathlib.Path(__file__).resolve().parents[2]


class RabbitHostTests(unittest.TestCase):
    def native(self,name):
        source=(ROOT/'src/actors'/name).read_text()
        return rabbit_host.transform(ROOT,hostgen.transform(source)[0],name)

    def test_rom_callback_roster(self):
        rabbit,key=rabbit_host.inventory(ROOT)
        self.assertEqual(len(rabbit),16)
        self.assertEqual(len(key),4)
        self.assertEqual(rabbit['data_ov085_02130074'],'StateIdleInit')
        self.assertEqual(rabbit['data_ov085_0213008c'],'StateCaughtMain')
        self.assertEqual(key['data_ov085_0213017c'],'func_ov085_0212d038')

    def test_native_rabbit_dispatch(self):
        native=self.native('daMip_c.cpp')
        self.assertNotIn('fn(thiz)',native)
        self.assertNotIn('c->**q',native)
        self.assertIn('state->a.fn(this)',native)
        self.assertIn('state->b.fn(this)',native)
        self.assertEqual(native.count('pc_sharedfile_handle_construct(&data_ov085_'),7)
        for method in rabbit_host.inventory(ROOT)[0].values():
            self.assertIn('return self->'+method+'();',native)

    def test_native_key_dispatch(self):
        native=self.native('daObj_Mip_Key_c.cpp')
        self.assertIn('c->mState->enter(c)',native)
        self.assertIn('mState->execute(this)',native)
        self.assertNotIn('data_ov085_02130184',native)
        self.assertIn('data_0209caa0[1] |= 0x40;',native)


if __name__=='__main__':unittest.main()
