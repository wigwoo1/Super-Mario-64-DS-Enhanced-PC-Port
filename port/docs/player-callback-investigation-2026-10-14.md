# Player callback ABI investigation — 2026-10-14

Status: PARTIALLY VERIFIED. No new runtime fix is accepted in this checkpoint.
The two experimental layout changes were removed; copies remain under
build/abi-investigation for review.

## Measured failure

The native x86 compiler probe build/abi-investigation/measure_player.exe reports:

    Player=1896 member=4 State=12 main=4 cleanup=8

Player allocation size remains 0x768, but its callback records do not match
the recovered DS layout. DS member pointers are eight bytes, and Player::State
uses 24 bytes with main at +8 and cleanup at +16. Native MSVC currently uses
four-byte member pointers, State size 12, main at +4, cleanup at +8. Consequently
table indexing can select a zero adjustment word as a function. Merely replacing
function addresses is insufficient.

The recovered __sinit_ov002_021019d0.c at lines 2184–2195 copies seven two-word
records into data_ov002_0211075c. The locally extracted ROM and ov002 symbol map
prove their order:

| Index | Constant | Function |
| --- | --- | --- |
| 0 | 0210a48c | Player::func_ov002_020c75f0 |
| 1 | 0210a46c | Player::func_ov002_020c7350 |
| 2 | 0210a3ac | Player::func_ov002_020c71e0 |
| 3 | 0210a074 | Player::func_ov002_020c7194 |
| 4 | 0210a334 | func_ov002_020c72a4(void *self) |
| 5 | 0210a404 | Player::func_ov002_020c70ac |
| 6 | 0210a3f4 | Player::func_ov002_020c6fe4 |

All seven ROM adjustment words are zero. Six are recovered native member
methods; index 4 is a recovered C-style worker and needs a calling-convention
adapter if invoked as a native member. ROM addresses must be replaced before
native dispatch; copying ROM bytes does not relocate function addresses.

## Two focused attempts

1. __multiple_inheritance on the Player definition: compiler C2292 rejects the
   declared representation because the actual class has single inheritance.
2. Scoped pointers_to_members(full_generality, multiple_inheritance) with a
   best_case restore after Player: compiler C2285 rejects changing the pointer
   representation after it has been determined. This attempt rebuilt headers
   used by other classes, demonstrating the need to audit its scope carefully.

Stopped these approaches according to the user's two-attempt rule. Neither is
left enabled. Documentation for the compiler mechanisms:
[inheritance keywords](https://learn.microsoft.com/en-us/cpp/cpp/inheritance-keywords?view=msvc-170)
and [pointers_to_members](https://learn.microsoft.com/en-us/cpp/preprocessor/pointers-to-members?view=msvc-160).

## Recommended next work

Use Sol High to design a consistent PC representation for recovered Player
state and callback records, preserving DS offsets and dispatch semantics while
relocating real function addresses. Consider an explicit two-word host record
and typed dispatch adapters to avoid compiler-wide pointer-layout changes.
Audit all native Player state calls and constant-pool copies, including null
checks and cleanup veto semantics. Add layout and actual callback-dispatch
regressions, then test the complete scene transition with frame capture.

The latest actual runtime evidence remains resume83: access violation through
a null level-entry callback. Full gameplay and the requested PC features remain
unverified. This is not an appropriate Luna handoff milestone yet.

## Restored-build checkpoint

After reverting both experiments, the native game and smoke_stage_layout build
again; all five focused CTest checks pass. Runtime resume84 reproduces the null
callback fault and exits -1073741819 after 16.16 seconds. Its screenshot was
displayed to the user. No new repair was attempted after the two failures.

The user corrected the model assumption: Sol Medium was selected throughout
this investigation. Recommend Sol High for the next callback representation
repair. The earlier assumption that Luna was selected was incorrect.
