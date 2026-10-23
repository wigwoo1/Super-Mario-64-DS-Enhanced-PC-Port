"""Host-only startup adaptation for original castle drainage actors."""
import re

def transform(text,water=False):
    names=('C0WaterModelFilePtr','C0WaterCollisionFilePtr') if water else ('C0SwitchModelFilePtr','C0SwitchCollisionFilePtr')
    for name in names:
        pattern=rf'{name}\(unsigned int (\w+)\) \{{ pc_sharedfile_handle_construct\(this, \1\); \}}'
        text,n=re.subn(pattern,lambda m:f'{name}(unsigned int {m[1]}) {{ words[0]={m[1]};words[1]=0; }}',text)
        assert n==1,name
        old=f'~{name}() {{ pc_sharedfile_handle_destroy(this); }}';assert old in text;text=text.replace(old,f'~{name}() {{}}')
    stem='daObjC0Water_c' if water else 'daObjC0_Switch_c'
    signature=f'int {stem}::InitResources()\n{{';assert signature in text
    handle_model='data_ov012_021124d0' if water else 'data_ov012_021124a8'
    handle_clsn='data_ov012_021124c8' if water else 'data_ov012_021124a0'
    file_model,file_clsn=(1451,1452) if water else (1449,1450)
    init=f'\n    static bool ready=false;if(!ready){{{handle_model}.Construct({file_model});{handle_clsn}.Construct({file_clsn});ready=true;}}'
    text=text.replace(signature,signature+init,1)
    if water:
        text=text.replace('extern BTA_File      data_ov012_02111c24;','void *pc_basement_animation();')
        assert 'extern BTA_File      data_ov012_02111c24;' not in text
        text=text.replace('data_ov012_02111c24','(*static_cast<BTA_File *>(pc_basement_animation()))')
    return text
