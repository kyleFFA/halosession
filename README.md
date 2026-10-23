# Session

Session is a focused Windows practice overlay for Halo: The Master Chief Collection, based on Poorboys Checkpoint Manager and Halo Checkpoint Manager.

This is the **developer source tree** for [kyleFFA/halosession](https://github.com/kyleFFA/halosession). It is not a runnable player download. The source has been uploaded; no player release has been published.

## Current beta

The accepted Quick Menu input/action baseline now has a circular ringworld, floating navigation, compact centered pages, configurable wheel colors/opacity, and replaceable artwork. Garden, Aurora and Autumn skins are included alongside embedded Session V1. See [skin authoring](skins/README.md).

The user reports that the seasonal GUI is cohesive and the wheel works excellently. Startup appearance uses the captured live palette, placement and size, with the requested **50% background opacity** and **1.10x gentle cloud drift speed**. Travel amplitude is unchanged. Appearance edits and skin selection remain local to the loaded instance; see [defaults and validation](docs/APPEARANCE-DEFAULTS.md).

The user previously accepted PS5 operation through **XInput slot 0**. Xbox XInput and mapped PlayStation controllers use that input route; native DirectInput remains unverified. The separate controller-detection candidate is excluded. Gameplay actions, binding logic and session safeguards retain the accepted baseline. See [provenance](docs/BASELINE.md).

## Developers

Read [BUILDING.md](BUILDING.md) for the Windows x64 toolchain, pinned dependencies, native build and offline tests. Both developers should use this baseline and the same separately supplied runtime dependency versions.

The repository includes the native launcher, **injector/bridge source**, overlay, physics integration, native H2 tools, controller wheel, tests and source assets. The h3sb physics DLL and packed helper are external runtime dependencies; their implementation is not present as buildable source here. Excluding compiled dependencies from Git does **not** remove physics from the working app or authorize a physics-less player release.

## Players

A player download should be a separate, complete ZIP with the launcher, overlay, injector/bridge, speedhack, required physics DLL/helper bundle, static runtime data and notices. It should omit development files, debug symbols, logs, crash dumps, recordings and personal settings. **No new public download has been created.** Release is held for contributor changes/feedback and subsequent owner approval. See the [player release plan](docs/PLAYER-RELEASE.md).

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
