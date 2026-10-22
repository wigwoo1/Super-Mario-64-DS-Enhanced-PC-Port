from pathlib import Path
import sys,unittest
import copy
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from native_kit_data import DATA_SOURCES,local_source,validate_generated_inputs,validate_manifest

class KitDataGuard(unittest.TestCase):
    def test_reviewed_generated_data_stays_local(self):
        for name in ('sound/stage_sound_data.asm','camera/camera_native_data.asm',
                     'camera/camera_native_data.cpp','minigame/minigame_resources.asm'):
            path='CMakeFiles/sm64ds_pc.dir/pc-boot-src/'+name+'.obj'
            for spelling in (path,path.replace('/','\\')):
                self.assertEqual(local_source(spelling),'build/port/pc-boot-src/'+name)
        self.assertEqual(len(DATA_SOURCES),15)

    def test_unreviewed_generated_files_are_rejected(self):
        for name in ('new_data.asm','new_data.cpp','callbacks.cpp'):
            with self.assertRaisesRegex(ValueError,'local reconstruction recipe'):
                validate_generated_inputs(['CMakeFiles/sm64ds_pc.dir/pc-boot-src/new/'+name+'.obj'])

    def test_code_and_existing_separately_classified_inputs_continue(self):
        validate_generated_inputs(['CMakeFiles/sm64ds_pc.dir/pc-boot-src/src/engine/Sound.cpp.obj',
            'CMakeFiles/sm64ds_pc.dir/host-src/dsmt_data.asm.obj'])

    def test_manifest_rejects_precompiled_data_and_incomplete_recipes(self):
        metadata={'format':2,'local_objects':[{'source':source,'object':f'local-{i:04d}.obj',
            'source_sha256':'a'*64} for i,source in enumerate(sorted(DATA_SOURCES))],
            'prebuilt':[{'path':'.native-kit/objects/0001.obj',
                         'origin_object':'CMakeFiles/sm64ds_pc.dir/main.cpp.obj'}]}
        validate_manifest(metadata)
        bad=copy.deepcopy(metadata);bad['local_objects'].pop()
        with self.assertRaisesRegex(ValueError,'roster'):validate_manifest(bad)
        bad=copy.deepcopy(metadata)
        bad['prebuilt'][0]['origin_object']='CMakeFiles/sm64ds_pc.dir/pc-boot-src/camera/camera_native_data.asm.obj'
        with self.assertRaisesRegex(ValueError,'cannot be prebuilt'):validate_manifest(bad)
        bad=copy.deepcopy(metadata);bad['local_objects'][0]['object']='../escape.obj'
        with self.assertRaisesRegex(ValueError,'object name'):validate_manifest(bad)
        bad=copy.deepcopy(metadata);bad['format']=1
        with self.assertRaisesRegex(ValueError,'format-2'):validate_manifest(bad)

if __name__=='__main__':unittest.main()
