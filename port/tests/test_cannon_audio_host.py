from pathlib import Path
import sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import cannon_host
class CannonAudioHost(unittest.TestCase):
    def test_original_cues_without_legacy_forced_stop(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'src/game/actors/d_a_cnn.cpp').read_text()
        result=cannon_host.native_sound_lifetime(source)
        self.assertNotIn('audio_stop_effect(3,0x14c,4)',result)
        self.assertEqual(source.count('Sound::PlayBank3('),result.count('Sound::PlayBank3('))
        self.assertIn('Sound::PlayBank3(0x14e,',result)
        with self.assertRaisesRegex(ValueError,'anchor'):
            cannon_host.native_sound_lifetime(source.replace('audio_stop_effect(3,0x14c,4)','changed()'))
if __name__=='__main__':unittest.main()
