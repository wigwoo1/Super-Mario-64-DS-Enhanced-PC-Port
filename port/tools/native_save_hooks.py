"""Keep retail save dialogs from reporting success after native disk failure."""
def transform(text):
    call='_ZN8SaveData15SaveCurrentFileEv();'
    if text.count(call)!=2:raise ValueError('Expected star and pause save call sites')
    star='''{ if (!_ZN8SaveData15SaveCurrentFileEv()) {
                data_0209f2e0 = 0; data_0209f2d4 = 6;
                pc_report_save_write_error(); return;
            } }'''
    pause='''{ if (!_ZN8SaveData15SaveCurrentFileEv()) {
                data_0209f2e0 = 0; data_0209f2c4 = 2;
                data_0209f22c = data_0208ee44 << 3; data_0209f2b8 = 0;
                pc_report_save_write_error(); return;
            } }'''
    before,between,after=text.split(call)
    return 'extern "C" void pc_report_save_write_error();\n'+before+star+between+pause+after
