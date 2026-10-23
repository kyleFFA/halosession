# Separate player release plan

Developers need source, tests and build instructions. Players need a complete runnable download. Keep those deliverables separate.

## Complete runtime package

A later reviewed `Session-<version>-win-x64.zip` should contain:

- `Session.exe`, `PCMInternal.dll`, `HCMInterproc.dll`, `PCMSpeedhack.dll` and the compatible pointer XML.
- **The required h3sb physics DLL, packed helper, its complete dependency subtree and necessary static data.** Preserve the physics functionality; never call a package complete when these dependencies are missing.
- Authored static spot data needed by retained features, after its distribution status is checked.
- Original manager license, third-party/runtime notices, credits and concise player instructions.

The accepted working distribution currently provides these dependencies. Its core hashes are documented for compatibility. Session's injector/bridge source remains in this repository; the h3sb implementation is the component not available here as buildable source.

## Binary redistribution review

The manager MIT notice includes Poorboy's changes and physics mod, which is relevant evidence; it is not a complete inventory of every component in the opaque runtime/helper bundle. Unknown provenance is **not** a determination that redistribution is prohibited.

Before public upload, establish the actual terms/source for the h3sb DLL/helper, any extracted or externally supplied physics/game payloads and the helper's Python/Tcl/Tk/PyInstaller/native dependencies. Retain the applicable notices and satisfy their terms. If redistribution is allowed, keep the compatible binaries in the player release assets. If some component cannot lawfully be bundled, document an authorized upstream prerequisite and validate the assembled app; do not provide a misleading physics-less ZIP.

A private developer repository, if later chosen, does not automatically provide a public player download destination. Decide the download location and visibility explicitly at publication time. No visibility changes or release creation have been made here.

## Omit development and personal data

Do not package source folders, build intermediates, debug symbols, installer caches, logs, crash dumps, memory/tick captures, personal settings, generated stop flags or recorded `h3sb/films/*.trk` sessions. The current local runtime must not be copied wholesale without classification. Keep debug symbols separately for maintainers.

Use reviewed defaults rather than the developer's active switch files. Required static helper/data dependencies must be retained even if they are compiled files. Embedded Session artwork does not require distributing art-review PNGs separately unless a runtime path actually reads them.

## Release validation

Assemble into a new empty staging directory without touching the working/default/rollback packages. Check the complete dependency inventory, hashes, licenses and absence of personal data. Then, with explicit authorization, smoke-test on a fresh session: connect/injector startup, physics/helper startup, retained physics controls, controller wheel/movement/camera release, native H2 0.2 vs H3/ODST/Reach 0.4 movement, title transitions, no co-op/multiplayer menu pause and normal shutdown. Test on a machine without the developer toolchain.

No new ZIP is declared runnable until these checks pass. No release has been assembled, published or promoted during source preparation.
