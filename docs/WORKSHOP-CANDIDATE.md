# Workshop candidate: dashboard, controls and full controller gameplay

> Historical candidate report. This implementation is included in the user-accepted source; see [current acceptance status](USER-TEST-STATUS.md). Statements below about pending acceptance or commit authorization describe the earlier candidate stage.

One separate private candidate, based on the camera/radial preview. Existing source, working previews, defaults and rollback packages are preserved. No new navigation buttons or screens were added. No Session launch, injection, commit, push, public release or default promotion was performed for this workshop.

## Where the controls are

- **Practice** renders the live physics dashboard directly, without the former Practice dropdown. The existing Poorboy attribution stays with the dashboard.
- **Control > Controls** contains Arm statistics and all 28 original downstream physics controls, in their original order. The Arm statistics popup, Refresh and Reset retain their callbacks. Original Halo 2's separate native group retains its nine actions here; it has no existing h3sb dashboard telemetry, so Practice shows a neutral unavailable explanation for that title.
- **Appearance > Menu and UI settings** contains the ten former Control menu/UI root entries, including their unchanged nested settings. Gameplay, binding and setting values were not redefined by routing them.
- **Appearance > Full controller gameplay with main menu** is the new opt-in input choice, initially OFF. The existing left-stick movement and optional right-stick camera choices remain available; they are disabled while full mode is checked.

The ordered source inventory is included in WORKSHOP-CONTROL-INVENTORY.json. Practice, Control, Credits and Quick Menu remain the same four navigation choices. Further discoverability polish remains future work.

## Dashboard semantics

The centered speed label and number are enlarged, and the bar is approximately twice its previous thickness. A fixed numeric field with tabular digit cells keeps the row and following content stable as the number changes. Long values shrink within that field. Bounce State occupies a large centered line below the bar: ON is green; OFF is red with an outline for readability.

Speed is the existing raw body-velocity magnitude in world units per second. The separate pass-threshold caption remains diagnostic. ON/OFF uses only the existing armed flag; no new bounce detection, physics changes or crouch behavior was introduced. Unloaded, missing/stale/future telemetry, flying, disabled physics, missing player and invalid values show neutral **Bounce State: Unavailable**, with an explanation, rather than an authoritative red OFF. Stale means 1.5 seconds without a valid update.

The small/corner and maximum-text fixtures verify that the speed and Bounce State stay visible and centered. At maximum text size, lower diagnostic/map/attribution rows can require scrolling. The resize handle remains usable.

## Input priority and controller coverage

Full mode passes both sticks, game buttons and triggers through the selected verified XInput route while the standard menu is open and eligible. Xbox pads and PlayStation pads presented through an XInput mapping use this route. This does not claim native PlayStation/DirectInput full-menu passthrough: that path remains blocked because its complete neutral mapping is not verified. Existing route, title/player/session and authority checks still apply.

The Session UI remains mouse-operated. Gamepad UI navigation and activation are disabled. Keyboard/mouse gameplay is blocked while this passthrough mode is active. Existing controller menu-toggle and radial-bound inputs are reserved from gameplay so a UI press cannot also jump or fire; their bindings still work. Other existing controller tool-hotkey blocking remains in effect while the menu owns input. Input preferences are runtime gameplay choices and remain excluded from Saved Skins.

Ordinary mouse-driven widgets can coexist with full gameplay; they cancel radial eligibility while being used. Text entry, popups/errors and invalid gameplay context suspend passthrough. Enable/re-entry requires neutral sticks and released buttons/triggers. This prevents held input from leaking into gameplay or selecting a restored UI control.

The radial ALWAYS owns the right stick exclusively while active. It suppresses camera look and other action buttons/triggers; left movement continues only when allowed by the movement/full choice and its verified gate. Releasing dispatches at most one eligible action; centering/canceling dispatches none. The normal menu is temporarily hidden and then restored with its previous tab, scroll, size, appearance and input choice. Look waits for recentering. Focus loss, disconnect, loading/death/title/player/session changes, timeout and explicit close cancel pending selection. A held mouse click cannot activate the restored menu. These transitions were simulated in fixtures; new live acceptance is still pending.

Full passthrough and radial capture suppress automatic menu pause. The existing verified solo-H2 pause rule remains for blocking mode; multiplayer/co-op is not forced to pause. Manual pause behavior and core physics implementations are unchanged.

## Verification

Release x64 native overlay and launcher build passed after the final dashboard edit. Focused shared-policy tests passed full raw controller passthrough, on/off/release gates, reserved menu-toggle and radial controls, ordinary mouse interaction, hard UI/focus conflicts, disconnect and identity/title/death changes, timeout, one-shot release and keyboard/mouse/pause policy.

Production WARP rendering passed all 15 telemetry branches, stable digit and ON/OFF layout, thick bar, six size/text combinations, centered small/corner placement, actual Arm statistics popup Refresh/Reset callbacks, routed groups and Appearance callback, actual full-input mouse checkbox, blocked gamepad UI activation, gameplay exclusion from aesthetic serialization and actual mouse resizing. The production hook assembly fixture passed both-stick/button/trigger passthrough, reserved inputs, radial suppression, existing Xbox/native buffers and retirement.

Existing optional-camera/radial priority, original radial cancellation, verified-solo pause, background/font/layout/preset, captured appearance defaults and actual menu restoration regressions passed. The actual packaged Saved Skins were loaded/rendered through the production loader and retained movement, look and full-gameplay flags. Source auditing confirms retained control order, binding/setting definitions, core physics implementations and artwork.

All of these workshop results are offline/automated, not a claim of new live gameplay acceptance. Previews are offline fixture telemetry; the Controls picture uses representative fixture rows, not the complete live catalog. Use LIVE-CHECKLIST.txt for the focused live check.

## Prior live results and package preservation

For the previous combined preview, the user confirmed fonts, animated backgrounds, centering, left-stick movement, movement OFF restricts / ON permits, hotkeys and the other mods they tested. They also confirmed changing appearance then loading a Saved Skin restored it. Those reports do not establish every mod/title/route, preset backup/corrupt/portable-copy case or unreported input transition. The new dashboard, routing and full-controller behavior need their own live check.

Current binding XML and every existing Saved Skins file, including the user's Test1 preset, are copied byte-for-byte into this separate folder. Compatible runtime/helper/static-data files are included. Accepted immutable helper switch files initialize this candidate; the working preview's live helper state is untouched. HCMInterproc.dll and PCMSpeedhack.dll are unchanged. PDBs are in the separate symbols folder. PRIVATE-TEST-MANIFEST.json records hashes and scope; build-artifact-hashes.json covers every delivered runtime file except itself.
