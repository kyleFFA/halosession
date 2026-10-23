# Session

Session is a focused Windows practice overlay for Halo: The Master Chief Collection, based on Poorboys Checkpoint Manager and Halo Checkpoint Manager.

This is the **developer source tree**, prepared for [kyleFFA/halosession](https://github.com/kyleFFA/halosession). It is not a runnable player download. Nothing has been pushed or released during this preparation.

## Selected baseline

The source is the **original Quick Menu beta**, which the user reports working successfully with a PS5 controller through **XInput slot 0**. Controller brand does not determine the input API: mapped PlayStation controllers use the XInput option. Native DirectInput remains unverified. This report is not a test of every title, connection method or checklist case.

The newer controller-detection candidate is **not included**. It passed offline checks but has not been user-tested. It adds route discovery/setup guidance, diagnostics and a guard against acting while the game controller read is blocked. See [baseline and provenance](docs/BASELINE.md).

Future skin refinements are [parked in the backlog](docs/BACKLOG.md); none are implemented in this source preparation.

## Developers

Read [BUILDING.md](BUILDING.md) for the Windows x64 toolchain, pinned dependencies, native build and offline tests. Both developers should use this baseline and the same separately supplied runtime dependency versions.

The repository includes the native launcher, **injector/bridge source**, overlay, physics integration, native H2 tools, controller wheel, tests and source assets. The h3sb physics DLL and packed helper are external runtime dependencies; their implementation is not present as buildable source here. Excluding compiled dependencies from Git does **not** remove physics from the working app or authorize a physics-less player release.

## Players

A player download should be a separate, complete ZIP with the launcher, overlay, injector/bridge, speedhack, required physics DLL/helper bundle, static runtime data and notices. It should omit development files, debug symbols, logs, crash dumps, recordings and personal settings. **No new public download has been created.** See the [player release plan](docs/PLAYER-RELEASE.md).

## Licenses and publication review

The original [MIT license](LICENSE), [third-party notices](THIRD-PARTY-LICENSES.md) and source copyright notices are retained. Additional vendored notices are listed in [LICENSE-AUDIT.md](docs/LICENSE-AUDIT.md). The manager's MIT license alone does not establish redistribution terms for every external physics/helper component.

One inherited collision-viewer header still contains a private-branch warning even though its upstream version is publicly available in Harc's MIT-licensed repository. It is retained and flagged for review before public publication. No private research directories, captured game memory or game binaries are included.

The local repository is staged for review, without a new commit or push. Repository-local author name is `KyleFFA`; the account-provided GitHub no-reply email has been verified and configured locally. Write authentication has not been tested. No collaborator has been invited, access granted or visibility changed.

## Credits

- Poorboy: Poorboys Checkpoint Manager and the h3sb physics integration.
- Burnt-o (Maurice Dix) and Harc: Halo Checkpoint Manager and the underlying practice-tool infrastructure.
- Harc and Jagged: collision-viewer research and rendering contributions; original notices are preserved.
- The third-party authors identified in their source headers, licenses and notice files.
- KyleFFA (badstrafe).
