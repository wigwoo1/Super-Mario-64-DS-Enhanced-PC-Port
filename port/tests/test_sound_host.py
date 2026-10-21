from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import sound_host
class SoundHost(unittest.TestCase):
    def test_real_motion_sound_helper(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'src/engine/sound/Sound.cpp').read_text()
        result=sound_host.transform(source,'Sound.cpp')
        self.assertNotIn('audio_effect_timed(j1,j2,bank,60)',result)
        self.assertNotIn('embedded loop indefinitely',result)
        start=result.index('extern "C" void func_0201251c(')
        end=result.index('// @symbol Sound_PlayIfNotActive',start)
        body=result[start:end]
        self.assertIn('ntr::audio_effect(a,b,a==3?data_0208e428:-1);',body)
        self.assertLess(body.index('if(ntr::audio_ready())'),body.index('int r4 = func_02048a18(b, d);'))
        self.assertIn('func_02048af4(&data_0209b4a4, c, d);',body)
        with self.assertRaisesRegex(ValueError,'anchor'):
            sound_host.transform(source.replace('void func_0201251c(','void changed('),'Sound.cpp')
        self.assertEqual(sound_host.transform(source,'Other.cpp'),source)
if __name__=='__main__':unittest.main()
