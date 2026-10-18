struct P { int a, b; };
extern void func_02018e3c(struct P *buf, int x);
extern void func_02018474(struct P p, int b, int c);
void func_02018434(int a0, int a1, int a2){
#if defined(SM64DS_PLATFORM_PC)
  extern int pc_fs_read_path(const char *path, void *dst, unsigned int capacity);
  pc_fs_read_path((const char *)a0, (void *)a1, (unsigned int)a2);
#else
  struct P buf;
  func_02018e3c(&buf, a0);
  func_02018474(buf, a1, a2);
#endif
}
