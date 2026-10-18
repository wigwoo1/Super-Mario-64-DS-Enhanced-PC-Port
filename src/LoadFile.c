/* LoadFile at 0x0201816c
 *
 * Matched byte-for-byte with mwccarm 1.2/sp2p3 (arm9 main).
 */

#if defined(SM64DS_PLATFORM_PC)
extern void *pc_fs_load_file(unsigned int handle);

int LoadFile(int handle) {
    return (int)(unsigned long)pc_fs_load_file((unsigned int)handle);
}
#else
extern int func_0201818c(int handle, int mode);

int LoadFile(int handle) {
    return func_0201818c(handle, 1);
}
#endif
