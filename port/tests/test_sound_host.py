from pathlib import Path
import sys,tempfile,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import sound_host
class SoundHost(unittest.TestCase):
    def test_stage_sound_rows_share_original_byte_lanes(self):
        # The sandbox system temp can reject child creation/atomic writes.
        # Keep this synthetic fixture under the test's writable working folder.
        with tempfile.TemporaryDirectory(prefix='sound-host-test-',dir=Path.cwd()) as folder:
            root=Path(folder)
            (root/'extracted').mkdir()
            (root/'config/arm9').mkdir(parents=True)
            offset=sound_host.STAGE_SOUND_START-0x02004000
            table=bytes(range(52*3)) # Synthetic fixture, no cartridge data.
            (root/'extracted/arm9_dec.bin').write_bytes(bytes(offset)+table)
            symbols='\n'.join(f'{name} kind:data(any) addr:0x{sound_host.STAGE_SOUND_START+i:08x}'
                for i,name in enumerate(sound_host.STAGE_SOUND_LABELS))
            (root/'config/arm9/symbols.txt').write_text(symbols)
            paths=sound_host.write_sources(root,root/'generated')
            self.assertEqual(len(paths),1)
            source=Path(paths[0]).read_text().splitlines()
            values=[line for line in source if line.startswith('BYTE ')]
            self.assertEqual(values,[f'BYTE 0{x:02x}h' for x in table])
            for i,name in enumerate(sound_host.STAGE_SOUND_LABELS):
                label=source.index('_'+name+' LABEL BYTE')
                self.assertEqual(sum(x.startswith('BYTE ') for x in source[:label]),i)
                self.assertEqual(source[label+1],f'BYTE 0{i:02x}h')
            (root/'config/arm9/symbols.txt').write_text(symbols.replace('02075769','02075779'))
            with self.assertRaisesRegex(ValueError,'interior label'):
                sound_host.write_sources(root,root/'generated')
            (root/'extracted/arm9_dec.bin').write_bytes(bytes(offset)+table[:-1])
            with self.assertRaisesRegex(ValueError,'boundary'):
                sound_host.write_sources(root,root/'generated')
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
    def test_sdk_native_start_and_retail_fallback(self):
        root=Path(__file__).resolve().parents[2]
        source=(root/'src/engine/sound/SoundEngine.cpp').read_text()
        adapted=sound_host.transform(source,'SoundEngine.cpp')
        start=adapted.index('extern "C" int func_02051f1c(')
        end=adapted.index('// @symbol func_02051e60',start)
        body=adapted[start:end]
        self.assertLess(body.index('ntr::audio_effect('),body.index('func_02050c14)(a1)'))
        self.assertIn('reinterpret_cast<uintptr_t>(a0)',body)
        self.assertNotIn('*a0=',body) # No fake ARM player pointer in the handle.
        with self.assertRaisesRegex(ValueError,'anchor'):
            sound_host.transform(source.replace('int func_02051f1c(void *a0','int changed(void *a0'),'SoundEngine.cpp')
    def test_menu_handle_queries_follow_native_lifetimes(self):
        root=Path(__file__).resolve().parents[2]/'src/unnamed/ov007'
        for name in ('020bd9e4','020bdbcc','020bde2c','020bde70','020bdebc','020bdf60'):
            path=root/('func_ov007_'+name+'.c')
            adapted=sound_host.transform(path.read_text(),path.name)
            self.assertIn('ntr::audio_active(',adapted)
            self.assertIn('*static_cast<const unsigned *>(slot)!=0',adapted)
if __name__=='__main__':unittest.main()
