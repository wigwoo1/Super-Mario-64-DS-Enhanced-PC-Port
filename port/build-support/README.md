# Native data link metadata

These files contain symbol names, aliases and compiler declarations, without
ROM payload bytes. They seed the indirect data link closure on a fresh source
checkout. CMake retains an existing local history/cache when one is present.

`pc_link_data.json` records the native/ROM symbol spellings requested by the
current closure. `pc-game-link-requests.txt` contains only the relevant compiler
declarations from past unresolved-symbol diagnostics, with machine paths removed.

The actual data is regenerated privately from the user's own cartridge dump.
When the closure grows, update these metadata lists from a successfully linked
development build, review them, and verify a fresh local generation/relink.
