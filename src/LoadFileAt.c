/* LoadFileAt at 0x020182ac
 *
 * Matched byte-for-byte with mwccarm 1.2/sp2p3 (arm9 main).
 */

#if defined(SM64DS_PLATFORM_PC)
extern unsigned int pc_fs_load_file_at(unsigned int handle, void *dest);

int LoadFileAt(int handle, int dest) {
    return (int)pc_fs_load_file_at((unsigned int)handle,
                                   (void *)(unsigned long)dest);
}
#else
extern int func_02018270(int handle, int dest, int size);

int LoadFileAt(int handle, int dest) {
    return func_02018270(handle, dest, -1);
}
#endif
