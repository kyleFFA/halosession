# Baseline and provenance

Historical import: original Session Quick Menu beta, prepared October 22, 2026. The user reports successful PS5/XInput operation, including attempts to break the menu. This is user-reported live acceptance for that setup; native DirectInput, every game title and every transport are not thereby validated.

Upstream bases:

- Poorboys Checkpoint Manager `0c1a55230b333d229ccd08ef48854f8250f23449`: https://github.com/PoorboyTTV/PoorboysCheckpointManager
- Harc's HCM reference `240c32c20cbcedd016604c48f769284201db0ade`: https://github.com/harctheshark/HaloCheckpointManager---Harcs-Build
- Original HCM: https://github.com/Burnt-o/HaloCheckpointManager

The new controller-detection candidate was deliberately excluded. It adds discovery of other input routes, clearer labels, explicit route selection, logging and blocked-read cancellation. It has only offline acceptance; no tested binary/source substitution was made.

## History policy

The upstream Poorboy history contains 909 commits and historical compiled files, including `HCM3/keystone.dll` and `HCMInternal/HCMInternal/FW1FontWrapper/FW1FontWrapper.lib`. The source also contains historical personal-path comments. Importing that ancestry would defeat a source-only/privacy-clean upload.

This repository therefore starts a new sanitized source history. [The upstream contributor/commit ledger](upstream/history.tsv), exact base IDs, original licenses, per-file hashes and [the original upstream README](upstream/Poorboys-README.md) preserve attribution and traceability. The ledger omits author email addresses. It is provenance metadata, not a claim that the new repository retains Git ancestry. The upstream repositories remain the authoritative history.

## Original import preparation changes

- Kept runtime C/C++ and shader code from the tested baseline, including existing physics, controller selection/action, identity/authority, pause and hook-retirement logic.
- Redacted personal paths from comments in three collision-viewer headers; executable code is unchanged. The inherited private-branch warning is retained.
- Sanitized local generated-art reference paths in the prompt record; the two PNG asset bytes are unchanged.
- Removed non-build media/design entries for old unused DDS/audio/images and three stale IDE-only header references from the native projects. The tested resource script already embeds only the two Session PNGs. Compile units and resource inputs are unchanged.
- Added developer documentation, source-audit/build scripts, ignore rules, license supplements and a native-only solution. No new gameplay feature or binary rebuild was performed.
- The user subsequently specified the credit spelling `KyleFFA (badstrafe)`. Prepared source/UI test labels and notices use that spelling, in the same last position. No working binary was rebuilt for this change.
- Git normalizes text line endings; the local preparation comparison accounts for that without changing C/C++ logic.

Exact source hashes and the proposed file list are recorded in the local preparation report outside the Git payload.

## Subsequent GUI update

The accepted Quick Menu input/action source is retained. GUI changes add shared measured circular geometry, floating navigation, compact pages, artwork loading and Garden/Aurora/Autumn assets. The user accepted the seasonal GUI and wheel in a live session. Current defaults capture that instance's appearance, overriding background opacity to 50% and scaling gentle drift speed by 1.10. No controller-detection candidate, physics/action changes or ripple effect is included. See [validation](APPEARANCE-DEFAULTS.md).

`source-provenance.json` records the original sanitized import hashes, not hashes of subsequent commits. The original upstream ledger and notices remain unchanged. Artwork prompt records retain intermediate generation history; the unused intermediate `session-ringworld.png` is not distributed. The selected corrected ringworld and the circular fixture asset are included unchanged.
