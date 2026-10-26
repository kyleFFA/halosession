# Session OBS Beta — October 26, 2026

Release: [v2026.10.26-obs-beta](https://github.com/kyleFFA/halosession/releases/tag/v2026.10.26-obs-beta)

ZIP: [Session-2026.10.26-obs-beta-win-x64.zip](https://github.com/kyleFFA/halosession/releases/download/v2026.10.26-obs-beta/Session-2026.10.26-obs-beta-win-x64.zip)

Checksum file: [SHA256SUMS.txt](https://github.com/kyleFFA/halosession/releases/download/v2026.10.26-obs-beta/SHA256SUMS.txt)

ZIP SHA-256:

```text
23cde896b824154bd94fdfefc343d9bbf6bc447262ec6291754b9c38375e4681
```

## Setup

Extract the complete ZIP to a writable folder and keep its contents together. Start Steam MCC with anti-cheat disabled, run `Session.exe`, accept its administrator prompt, then choose Connect. Open the overlay with the grave-accent/backtick key left of 1. Configure controls before enabling practice features. Download the named ZIP rather than GitHub's automatic source archives.

No personal settings or Saved Skins are supplied. First run uses compiled defaults: physics, Fly, Keep Armed, Invulnerability, Speed Overlay, map grid and OBS bypass start off; the controller wheel starts disabled until configured. Settings persist beside this copy. Optional physics subsettings retain their compiled values but apply only when the corresponding feature is enabled.

For OBS exclusion, use Game Capture with Capture third-party overlays OFF, enable Bypass OBS Capture on Control, wait for a verified route and verify a recording yourself. Display/Window Capture are outside this route. Halo 3's grid uses the post-bloom frame renderer during exclusion, which can change its glow and edge coverage while preserving saved grid options.

## Identity and actual checks

Product source: [`f8f06918655847f437db6f5ffcd861e5e0006f7f`](https://github.com/kyleFFA/halosession/commit/f8f06918655847f437db6f5ffcd861e5e0006f7f). The accepted launcher and overlay are packaged byte for byte. All tracked native and launcher inputs match the accepted build tree after whitespace/line-ending normalization. The release tag adds documentation; it does not claim that the binaries were rebuilt from the documentation commit. Inherited binary file versions are separate from this calendar-based release tag.

- ZIP: 56,516,928 bytes; 1,187 files. CRC, safe paths, exact extracted file set and all file hashes passed.
- All 98 native images are Windows x64. Static dependency checks found no unresolved imports against the supplied runtime and Windows system DLLs.
- All 1,071 required physics/helper/static-data hashes match the accepted bundle and Poorboy's official 1.4.1 asset. Its asset digest was checked. [Runtime license review](LICENSE-AUDIT.md) records the matching helper libraries and included terms.
- The clean extracted folder passed a production defaults/hotkey check and WARP rendering with embedded/default artwork, three packaged skins and five center modes. Both external backgrounds decoded. It had no personal config, presets, logs, recordings or debug symbols, and the fixture left its file set/hashes unchanged.
- The user reported live success of these same product binaries in Halo 3, ODST and Reach. The cleaned ZIP was checked offline; no automatic Session/helper launch, game injection, fresh recording, or clean-machine live test was performed.

[Machine-readable verification](releases/2026.10.26-obs-beta.json) and the ZIP's `BUILD-INFO.json` / `PACKAGE-SHA256.json` retain identity and integrity details.

## Known limits

Halo 3 Invulnerability with H2-style physics can suppress rocket/grenade knockback. The damage-safe explosive-push fix is **not included**; ODST/Reach are not claimed fixed either.

Xbox/XInput controllers use the supported route. PlayStation controllers mapped to XInput use that route; native PlayStation/DirectInput coverage remains limited. The build targets Steam MCC 1.3528. The upstream physics runtime requires matching versions/tick settings among participating players and does not support co-op campaign/Firefight. Exhaustive hardware/title-transition coverage is not claimed.

Original MIT and third-party notices, additional native dependency notices and helper component terms are in the ZIP. Runtime dependencies are unchanged; no physics-less replacement was substituted.
