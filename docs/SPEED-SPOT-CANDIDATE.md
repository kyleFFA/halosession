# Large speedometer and bound Save/Back follow-up

> Historical candidate report. This implementation is included in the user-accepted source; see [current acceptance status](USER-TEST-STATUS.md). Statements below about pending acceptance or commit authorization describe the earlier candidate stage.

One separate private candidate based on the live-tested workshop. Existing packages, source, bindings and saved skins are preserved. No Session launch/injection, commit, push, upload, release or default promotion was performed.

## Diagnosis and input change

The user clarified the Reach result: **radial Save Spot and Back to Spot work**. The failure is specifically the separately bound controller hotkeys while the standard menu is open; neither produces a status message. The source explains this: HotkeyManager blocks every non-toggle gamepad hotkey when the menu captures input, and its ordinary worker checks the same condition again. This prevents those presses from reaching either spot event. The coordinate service itself was not established as faulty.

Save/Back controller bindings now enter the existing guarded action worker through a dedicated menu press event. The binding is read from the current hotkey definitions, not hard-coded to D-pad. Buttons, triggers, stick clicks and direction bindings use the same path; controller chords retain their normal all-held/new-press semantics. One press queues at most one coordinate action per frame, holding does not repeat, and that press never also reaches the ordinary hotkey worker. The configured controller components are reserved from gameplay while the main menu is open to prevent an action from also jumping/firing/pausing. Their normal gameplay use returns when the menu closes.

The radial and menu-toggle binding have priority over overlapping spot bindings. Radial capture and its 200 ms release guard prevent bound spot actions from firing. Text entry, popups/errors, active mouse widgets, focus loss, disconnect, stale polling, title/player/session changes, timeout and explicit close refuse/cancel pending work. The worker rechecks the selected device, living local-player identity, session/build/authority observation, action availability, menu state and cancellation revision before dispatch. The selected verified XInput route is required; Xbox and PlayStation mapped to XInput use it. Native DirectInput full-menu passthrough remains blocked. The radial need not be enabled for separately bound Save/Back presses on the verified XInput route.

All four existing title routes—original Halo 2, Halo 3, ODST and Reach—still dispatch the same Save/Back events to their existing per-title services. Native H2 movement, Force Teleport, the physics bridge, bookmark/teleport implementations, hotkey definitions and serialized settings are byte-identical to the workshop source. No checkpoint restoration was added. Original H2 retains its existing next-tick queue/expiry behavior when gameplay is paused; this follow-up does not introduce automatic resume or new authority exceptions. Full-controller gameplay otherwise retains its live-tested behavior. Other controller-bound tool actions retain their existing menu-blocking policy.

## Speedometer and lower utilities

The four-character reference number `9.99` spans about **40% of the inner-circle opening width**. It scales with menu size. The allocated field and row remain fixed as digits change; digits use tabular advances, the decimal uses its natural width, and longer values shrink to fit rather than moving the following rows. The general text-size preference still sizes the caption, Bounce State and other UI; the main number follows circle geometry so it remains prominent.

The Speed caption is smaller. Bounce State remains centered beneath the thick speed bar. The temporary Session title, Appearance, close and resize controls form a compact lower utility row on Practice. Drag the Session title to move the menu; the diagonal handle resizes it. No logo or navigation screen was added. Other tabs retain their existing utility placement. The dashboard rectangle is slightly narrower and taller, with its corners verified inside the measured opening for the selected skin.

Telemetry semantics are unchanged: raw speed in world units per second; ON/OFF uses the existing armed flag; stale/unloaded/flying/invalid contexts remain neutral Unavailable. At large text sizes, lower diagnostics and attribution may require scrolling; speed, Bounce State and utilities remain visible.

## Text rendering

The workshop loaded Segoe UI at 16 px and magnified those atlas glyphs for larger readouts. This is a confirmed scaling path that contributes to the reported pixelation; no claim is made that it was the only visual factor on the user's display.

The same Segoe UI and Segoe Print families remain. UI text is rasterized with 2× density and oversampling, with a denser variant for 2× framebuffer scaling. Larger labels use a separate higher-resolution face. Speed digits have a dedicated **384 px raster**, limited to numeric punctuation and digits, and are downscaled at the tested sizes. No giant number is produced by enlarging a 16 px glyph. The existing linear-filtered DX11 atlas path is retained; there is no font shader, new texture per frame or atlas rebuild on menu resizing.

In the WARP fixture using all production font choices, both the old and new atlases occupied 2048×2048 RGBA, **16 MiB of GPU texture allocation**. The new font build took about 23 ms in one run; that is startup/device-recreation work, not a per-frame cost. The measured 4K WARP frame median was about 0.26 ms, p95 1.30 ms, for this fixture's full render submission. These are local CPU-side fixture measurements, not an in-game GPU/FPS guarantee or a reliable old/new timing comparison. CPU font data/glyph bookkeeping is additional and was not separately profiled. Raw measurements are in FONT-QUALITY.json.

## Automated verification and live status

- Release x64 overlay and launcher build passed after the final product edits.
- Shared production policy tests passed all 24 supported gamepad input mappings, new-press/held behavior, coordinate event routing for all four supported title IDs, guarded queue and dispatch cancellation, UI/toggle/radial priority, reserved gameplay components, radial one-shot dispatch and exclusive right-stick capture.
- Production WARP tests passed 40%-opening reference digits, stable row/field across digit changes and ON/OFF, all existing telemetry branches, six small/large/text combinations, 4K and 2× framebuffer scaling, actual Appearance/close/drag/resize controls, font texture recreation and two fresh-context reloads. Atlas size stays fixed while resizing. The statistics popup and preset exclusions still pass.
- Controller policy and assembly-hook regressions passed; combined backgrounds/static/animation/font/layout/skins/preset regressions and actual menu/radial state restoration passed. The packaged presets are loaded/rendered with production code before delivery. SOURCE-AUDIT.json records the unchanged service/binding/physics boundaries.

The user live-confirmed that the workshop is good overall, full controller button passthrough works without issues, and Reach radial Save/Back works. Earlier combined font/background/centering/movement-toggle/hotkey/tested-mod and Saved Skins round-trip confirmations remain recorded in the historical report. **The new bound-hotkey fix, larger speedometer, utility placement and improved font rendering still need live verification.** No new title/controller/transition coverage is inferred from the automated fixtures.

Current binding XML and all saved presets, including Test1, are copied byte-for-byte. The runtime/helper/static-data closure is included; HCMInterproc.dll, PCMSpeedhack.dll and helper binaries remain unchanged. Immutable accepted helper switch files initialize the new candidate without writing the running package's live switches. Symbols are separate. Previews use offline fixture telemetry; historical reports describe older builds.
