# Developer setup

The current source includes the accepted menu/controller follow-ups and independent Speed Overlay. The native overlay and launcher passed Release x64 compilation using the installed pinned dependency cache. The repository build wrapper restores declared dependencies; fresh-machine reproduction remains a contributor check.

## Prerequisites

- Windows x64; Visual Studio 2022 or Build Tools with MSVC v143 and a Windows SDK.
- Git and an installed vcpkg toolchain. The script can use the vcpkg bundled with VS 2022 or `VCPKG_ROOT`.
- Python 3 for a few offline test drivers and the source audit.

The reference local build used MSVC `14.44.35207` and Windows SDK `10.0.28000.0`. The code uses C++23-era library features with `stdcpplatest`; older v143 installations may need an update. The pinned vcpkg baseline and vendored code are as important as the compiler family.

The selected native launcher/overlay build does not require the old WPF application or .NET 7. The old managed UI, game thumbnails and unrelated candidate copies are intentionally absent.

Dependencies are pinned by the retained per-project `vcpkg.json` baseline `564be70dade303e7f18e0f350c3e52fec85db11b`. Vendored source versions and local safetyhook accessor changes are documented in [VENDORED-SOURCES.md](docs/VENDORED-SOURCES.md). They are ordinary files, not missing Git submodules; do not replace them with unpatched upstream checkouts.

## Build

From the repository root, in PowerShell:

```powershell
./tools/build-source.ps1
```

This restores the declared dependencies and builds the internal DLL and native launcher. It does not launch Session, connect to MCC or assemble a player release.

To also compile the supplied injector/bridge and speedhack projects:

```powershell
./tools/build-source.ps1 -BuildSupportLibraries
```

Their source is supplied, but newly rebuilt support DLLs have not been demonstrated byte-identical or runtime-equivalent to the protected DLLs in the accepted package. Do not silently replace the accepted runtime with them.

If vcpkg is elsewhere, supply `-VcpkgRoot <path>`. Use `-Rebuild` only when needed. The script writes ignored build logs and products locally. Existing project files retain the historical output directory `HCMExternal/bin/x64/Release/net7.0-windows`; this directory name does not mean the managed application is required.

The appearance-defaults overlay and launcher built successfully with the installed pinned dependencies. The focused defaults fixture passed compiled palette checks, actual 50% WARP blending, placement/responsive fitting and drift speed/amplitude checks. The local build reused the installed dependency cache; the published wrapper restores dependencies. Fresh-machine reproducibility remains to be checked by contributors.

## Runtime dependencies

The compiled source alone is not a complete usable distribution. Obtain the compatible h3sb physics DLL, helper and required static data through a lawful separately supplied dependency bundle. Keep that bundle **outside Git**, for example in an ignored `runtime/` directory. [Runtime dependency hashes](docs/runtime-dependencies.json) identify the accepted local versions; hashes are compatibility identifiers, not redistribution permission.

`HCMInterproc` supplies Session's injector/connection source. `Halo2Physics.cpp` and `h3sb_api.h` supply the integration layer. The compiled h3sb implementation and packed helper are distinct external dependencies. Their absence from the source upload does not justify omitting them from a player package.

No script here downloads, starts or injects the opaque runtime automatically. Resolve the dependency distribution terms and perform a clean-package smoke test before a public release. The complete existing working package remains the current fallback.

The complete static helper/physics/spot-data hash inventory is in [runtime-data-manifest.json](docs/runtime-data-manifest.json). Both developers can check an independently obtained bundle without loading it:

```powershell
python tools/verify-runtime.py <runtime-root>
```

This checks compatible files only. It neither copies binaries into Git nor grants redistribution rights. Runtime-generated films, switch states, logs and stop flags are outside this static inventory.

## Offline tests

Set `VSDIR` to the VS installation directory returned by `vswhere`, then run from a developer shell or with the provided scripts:

```powershell
cmd /c tools\test-quick-menu.cmd
cmd /c tools\test-quick-menu-pause.cmd
cmd /c tools\test-h2-movement.cmd
cmd /c tools\test-lean.cmd
```

After a native build, `test-quick-input-hook.cmd`, `test-module-lifecycle.cmd`, `test-sketch-ui.cmd`, `test-quick-wheel-view.cmd` and `test-message-render.cmd` exercise local fixtures and production rendering/hook ownership. Run module-lifecycle before message-render because it creates shared fixture objects. WARP tests produce ignored offline PNG previews.

The automatic legacy scripts that tried reading a running game are not included. Some retained test programs expose explicit read-only diagnostic arguments; ordinary no-argument test runs use fixtures. A live-controller checklist is in [controller-testing.txt](docs/controller-testing.txt).

## Collaborating

Each developer should clone this repository, use the same baseline/dependency hashes, work on a branch and review changes through pull requests. No contributor account or access level has been selected or invited during this preparation. Player ZIPs and development symbols should remain separate artifacts.

## Appearance and skin fixtures

Run `cmd /c tools\test-appearance-defaults.cmd` for the current focused defaults check. `test-skin-loading.cmd` and `test-seasonal-skins.cmd` cover skin validation/fallback and production rendering. `test-circular-skin.cmd` measures isolated rendering cost; those measurements are not in-game performance promises. Tests use the installed dependency path configured by the native project under `%LOCALAPPDATA%/HaloPracticeBuild/HCMInternal/x64-windows-static`.

Before the defaults change, the seasonal renderer, loader, controller hook/core and solo-H2 pause fixtures passed. Those broader checks were not repeated for this appearance-only update. Some older UI fixtures encode the earlier startup appearance; explicitly select a fixture appearance when comparing historical output. No additional in-game acceptance cycle was required for this update.

## Current UI regression fixtures

With `VSDIR` configured as above, the focused scripts exercise production rendering and input policy:

```powershell
cmd /c tools\test-combined-preview.cmd
cmd /c tools\test-menu-camera-priority.cmd
cmd /c tools\test-menu-camera-render.cmd
cmd /c tools\test-workshop-input.cmd
cmd /c tools\test-workshop-render.cmd
cmd /c tools\test-menu-spot.cmd
cmd /c tools\test-speed-overlay.cmd
cmd /c tools\test-speed-overlay-settings.cmd
```

The render fixtures use software WARP and write generated output to ignored `out/`. The Speed Overlay fixture writes its sizes/timings to `out/speed-overlay-validation.json`. Its settings fixture uses the production settings and event registrations. Local snapshot-comparison audits also passed before acceptance; those require private sibling candidate trees and are not part of the portable test suite. No script above launches or injects Session.
