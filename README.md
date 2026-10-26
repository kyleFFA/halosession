# Session

Session is a focused Windows practice overlay for Halo: The Master Chief Collection, based on Poorboys Checkpoint Manager and Halo Checkpoint Manager.

This is the **developer source tree** for [kyleFFA/halosession](https://github.com/kyleFFA/halosession). Compiled player downloads are provided separately through GitHub Releases.

## Download the compiled beta

Download [Session-2026.10.26-obs-beta-win-x64.zip](https://github.com/kyleFFA/halosession/releases/download/v2026.10.26-obs-beta/Session-2026.10.26-obs-beta-win-x64.zip) from the [Session OBS Beta release](https://github.com/kyleFFA/halosession/releases/tag/v2026.10.26-obs-beta), extract the whole ZIP, then run `Session.exe`. The release includes the required physics/helper runtime, artwork and notices. GitHub's automatic Source code archives are developer files.

Start Steam MCC with anti-cheat disabled, choose Connect in Session, and use the backtick key (left of 1) for the menu. This download starts with clean compiled defaults and saves settings beside its own copy. See [release details, checksum and limitations](docs/RELEASE-2026.10.26.md).

**Known issue:** Halo 3 Invulnerability with H2-style physics can suppress rocket/grenade push. That fix is not included in this accepted OBS build.

## Current beta

The user accepted the current build on October 25, 2026 and authorized committing its source. It includes the circular ringworld and seasonal skins, selectable animated or static center backgrounds, adjustable text and centered pages, and portable Saved Skins. See [skin authoring](skins/README.md).

Practice shows the prominent speed readout and bounce status. Control groups gameplay actions and Arm statistics, while Appearance groups menu and UI settings. The independent **Speed Overlay** has its own visibility hotkey, size, color, opacity and position, plus an explicit drag/lock mode. Its locked display remains visible with the menu closed without capturing input. See [Speed Overlay behavior and validation](docs/SPEED-OVERLAY-CANDIDATE.md).

Optional menu movement, camera and full controller gameplay share the guarded controller route; the radial wheel retains exclusive right-stick ownership. Separately bound Save Spot/Back to Spot controller presses work with the menu open through the existing guarded action worker. Xbox XInput and PlayStation controllers mapped to XInput use this route; native PlayStation/DirectInput coverage remains limited. Existing bindings and gameplay safeguards are retained.

[Live test status](docs/USER-TEST-STATUS.md) distinguishes user-reported acceptance from automated fixtures. The compiled beta uses this accepted source; its clean ZIP has separate offline packaging checks.

Control includes **Bypass OBS Capture**, with dynamic compatibility checks and capture-route status. The verified route draws Session after OBS Game Capture, including Halo 3's map grid. The user reported successful exclusion in Halo 3, ODST and Reach. Halo 3's excluded grid uses the post-bloom frame renderer, so its glow can differ. See [capture behavior and validation](docs/OBS-CAPTURE.md). Settings persist beside each package without a shared settings fallback.

## Developers

Read [BUILDING.md](BUILDING.md) for the Windows x64 toolchain, pinned dependencies, native build and offline tests. Both developers should use this baseline and the same separately supplied runtime dependency versions.

The repository includes the native launcher, **injector/bridge source**, overlay, physics integration, native H2 tools, controller wheel, tests and source assets. The h3sb physics DLL and packed helper are external runtime dependencies; their implementation is not present as buildable source here. Excluding compiled dependencies from Git does **not** remove physics from the working app or authorize a physics-less player release.

## Players

A player download should be a separate, complete ZIP with the launcher, overlay, injector/bridge, speedhack, required physics DLL/helper bundle, static runtime data and notices. It should omit development files, debug symbols, logs, crash dumps, recordings and personal settings. The owner authorized this compiled beta release on October 26, 2026. See the [player release record](docs/PLAYER-RELEASE.md).

## Licenses and publication review

The original [MIT license](LICENSE), [third-party notices](THIRD-PARTY-LICENSES.md) and source copyright notices are retained. Additional vendored notices are listed in [LICENSE-AUDIT.md](docs/LICENSE-AUDIT.md). The manager's MIT license alone does not establish redistribution terms for every external physics/helper component.

One inherited collision-viewer header still contains a private-branch warning even though its upstream version is publicly available in Harc's MIT-licensed repository. The comment and public MIT provenance are documented in the license audit. No private research directories, captured game memory or game binaries are included.

The developer source was uploaded with the repository owner's authorization. No collaborator has been invited, access granted or visibility changed as part of this upload. Contributors should configure their own Git author identity before committing.

## Credits

- Poorboy: Poorboys Checkpoint Manager and the h3sb physics integration.
- Burnt-o (Maurice Dix) and Harc: Halo Checkpoint Manager and the underlying practice-tool infrastructure.
- Harc and Jagged: collision-viewer research and rendering contributions; original notices are preserved.
- The third-party authors identified in their source headers, licenses and notice files.
- KyleFFA (badstrafe).
