extern unsigned short data_020a0f1c;
int func_0203d7d4(int arg){
  int b;
  if (arg != 0) return 1;
  data_020a0f1c |= 0x80;
#if defined(_WIN32)
  /* The DS boot-info block is not mapped in the native PC process. */
  b = 0;
#else
  b = (*(volatile unsigned short*)0x27ffc40 == 2);
#endif
  return b != 0;
}
