//cpp
/* arm9/SaveData -- the save-file manager: cap-loss flags, file slots and the
 * minigame block, folded from fourteen legacy shards at .text
 * 0x02013ad4..0x02013e80. The span also carries the two free functions that
 * share this TU's concern: func_02013c84 copies a file block and either
 * records the character or forwards to SaveFile, and func_02013e64 zeroes the
 * whole save block. mwccarm emits .text in reverse source order, so the
 * definitions below run ROM-descending; the roster reads ROM-ascending.
 *
 *   PlayerLoseCap..CanPlayerHaveCap -- cap-loss flags on the live block.
 *   SaveCurrentFile..SetDefaultValuesMg -- file and minigame writes.
 *   func_02013c84 -- block copy + character record / SaveFile forwarder.
 *   EraseSaveFile..EraseAllSaveData -- slot and whole-block resets.
 *   func_02013e64 -- zero the save block.
 *
 * Every method is static: the ROM bodies take no `this` and reach the single
 * save block at 0x0209caa0 directly. The TU declares it once as a byte array:
 * the block outlives the declared SaveData fields (the active-slot byte sits
 * at +0x328, the minigame block at +0x44, the mask word at +0x48), and only
 * the array spelling puts `data_0209caa0 + 0x44` in the literal pool the way
 * the ROM has it -- `(u8*)&block + 0x44` compiles to base-register add and
 * drops the second literal. Where a named member was byte-neutral the source
 * uses it (CanPlayerHaveCap reads mCharacter, HasPlayerLostCap flags1).
 *
 * deslop leftovers:
 * - PlayerLoseCap keeps its raw `*(int*)(data_0209caa0 + 4)` read-modify-write:
 *   spelling it as `->flags1` through a cast makes mwccarm materialise the
 *   field address in a register first (add r3,r0,#4 / ldr r2,[r3]) instead of
 *   the ROM's two direct `ldr [r0,#4]` hits.
 * - SaveFile and ReadFileData keep their materialised-address forms: the
 *   `int* p = ...; *p = *p | ...` two-step is what the ROM compiled (add then
 *   ldr/orr/str through the register). Folding the offset into the store
 *   changes the instruction -- the shards documented this against
 *   plan-cpp-language-mode.md Phase 6.
 * - func_02013c84 stays `extern "C"` under its cartridge name: callers in
 *   other TUs link it unmangled, and its body is a C-shaped helper that
 *   forwards to the member SaveFile.
 * - func_02013e64 likewise: the save-block wipe is a free function, kept C.
 * - func_0205a588 (SDK byte-fill) and CpuCopy8 (SDK unaligned copy) are asm
 *   primitives declared `void(void)` at their definitions; the parameterised
 *   local externs below are how callers spell them, as in every shard.
 */

#include "SaveData.h"

extern "C" {
extern u8 data_0209caa0[];            /* the live save block */
extern u8 data_0209f2d8[];            /* gate byte read by CanPlayerHaveCap */
extern void func_0205a588(void* dest, int value, int size);
extern void CpuCopy8(void* dst, void* src, u32 count);
}

// @symbol func_02013e64
extern "C" void func_02013e64(void)
{
    func_0205a588(data_0209caa0, 0, 0x32c);
}

// @symbol _ZN8SaveData16EraseAllSaveDataEv
u32 SaveData::EraseAllSaveData()
{
    u32 r4;
    r4 = SaveData::EraseSaveFile(0, (char*)data_0209caa0);
    r4 |= SaveData::EraseSaveFile(1, (char*)data_0209caa0);
    r4 |= SaveData::EraseSaveFile(2, (char*)data_0209caa0);
    SaveData::SetDefaultValuesMg((MinigameSaveData*)(data_0209caa0 + 0x44));
    return r4 | SaveData::SaveMinigames((MinigameSaveData*)(data_0209caa0 + 0x44));
}

// @symbol _ZN8SaveData16SetDefaultValuesEP12FileSaveData
void SaveData::SetDefaultValues(FileSaveData* fsd_)
{
    func_0205a588(((void*)fsd_), 0, 0x44);
    *(int*)((void*)fsd_) = 0x30303038;      /* magic8000 */
    *(unsigned char*)((char*)fsd_ + 0x41) = 3;
    *(int*)((char*)fsd_ + 8) |= 8;
    *(unsigned char*)((char*)fsd_ + 0x42) = 0;
}

// @symbol _ZN8SaveData12ReadFileDataEjP12FileSaveData
int SaveData::ReadFileData(u32 fileID, FileSaveData* dest)
{
    char* r5 = (char*)dest;
    s32 result = SaveData::ReadDataFromCart(r5, 0x44, fileID);
    if (result) {
        SaveData::SetDefaultValues((FileSaveData*)r5);
        if (result == 2)
            return 1;
        return 0;
    }
    {
        int* p = (int*)(r5 + 0xc);
        *p = *p & *(int*)((char*)&data_0209caa0 + 0x48);
    }
    return 1;
}

// @symbol _ZN8SaveData8SaveFileEjP12FileSaveData
int SaveData::SaveFile(u32 fileID, FileSaveData* data)
{
    int* ip = (int*)((char*)data + 4);
    *ip = *ip | 1;
    if (SaveData::SaveDataToCart((char*)data, 0x44, fileID) == 0)
        return 1;
    return 0;
}

// @symbol _ZN8SaveData13EraseSaveFileEjPc
int SaveData::EraseSaveFile(u32 fileID, char* saveArea)
{
    SaveData::SetDefaultValues((FileSaveData*)saveArea);
    if (SaveData::SaveDataToCart(saveArea, 0x44, fileID) == 0)
        return 1;
    return 0;
}

// @symbol func_02013c84
extern "C" int func_02013c84(u8 charID, FileSaveData* dest, s32 fileIndex, FileSaveData* src)
{
    CpuCopy8(dest, src, 0x44);
    if (fileIndex < 0) {
        data_0209caa0[0x328] = charID;
        return 1;
    }
    return SaveData::SaveFile(fileIndex, src);
}

// @symbol _ZN8SaveData18SetDefaultValuesMgEP16MinigameSaveData
void SaveData::SetDefaultValuesMg(MinigameSaveData * mg_)
{
  func_0205a588(((void*)mg_), 0, 0x2e4);
  *(int*)((void*)mg_) = 0x30303035;
}

// @symbol _ZN8SaveData16ReadMinigameDataEP16MinigameSaveData
int SaveData::ReadMinigameData(MinigameSaveData* dest)
{
    s32 result = SaveData::ReadDataFromCart((char*)dest, 0x2e4, 3);
    if (result) {
        SaveData::SetDefaultValuesMg(dest);
        if (result == 2)
            return 1;
        return 0;
    }
    return 1;
}

// @symbol _ZN8SaveData13SaveMinigamesEP16MinigameSaveData
int SaveData::SaveMinigames(MinigameSaveData* data)
{
    if (SaveData::SaveDataToCart((char*)data, 0x2e4, 3) == 0)
        return 1;
    return 0;
}

// @symbol _ZN8SaveData15SaveCurrentFileEv
int SaveData::SaveCurrentFile()
{
    if (!SaveData::SaveFile(data_0209caa0[0x328], (FileSaveData*)data_0209caa0))
        return 0;
    return SaveData::SaveMinigames((MinigameSaveData*)(data_0209caa0 + 0x44));
}

// @symbol _ZN8SaveData16CanPlayerHaveCapEv
int SaveData::CanPlayerHaveCap()
{
    if (((SaveData*)data_0209caa0)->mCharacter != 3) {
        int b = (int)(data_0209f2d8[0] == 1);
        if (b == 0)
            return 1;
    }
    return 0;
}

// @symbol _ZN8SaveData16HasPlayerLostCapEv
int SaveData::HasPlayerLostCap()
{
    if (!SaveData::CanPlayerHaveCap())
        return 0;
    return ((SaveData*)data_0209caa0)->flags1 & (0x1000000u << ((SaveData*)data_0209caa0)->mCharacter);
}

// @symbol _ZN8SaveData13PlayerLoseCapEv
void SaveData::PlayerLoseCap()
{
    if (!SaveData::CanPlayerHaveCap())
        return;
    *(int*)(data_0209caa0 + 4) = *(int*)(data_0209caa0 + 4) | (0x1000000u << data_0209caa0[0x41]);
}
