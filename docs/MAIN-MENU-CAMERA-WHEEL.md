# Optional main-menu camera and radial handoff

> Historical candidate report. This implementation is included in the user-accepted source; see [current acceptance status](USER-TEST-STATUS.md). Statements below about pending acceptance or commit authorization describe the earlier candidate stage.

This is a separate private candidate based on the working combined preview. The previous source/package/default/rollback builds are preserved. No commit, push, launch, injection or public release was performed.

## Input priority

1. Errors, modal/popups, text entry, active UI controls and mouse clicks have priority. They block/cancel radial input and gameplay passthrough.
2. Hold the configured radial binding: the wheel ALWAYS owns the right stick exclusively. Camera look is suppressed regardless of the camera checkbox. Within the main menu, activation/buttons/triggers are consumed so they cannot also act in game or fire controller-bound UI/tool hotkeys. Left movement continues only when the movement option is enabled and its verified gate is armed.
3. With the standard main menu open and no wheel, left-stick movement follows the existing movement checkbox. **Right-stick camera look with main menu** is a separate checkbox, OFF by default and enabled only alongside movement. Keyboard/mouse gameplay, triggers and other buttons remain blocked. Enabling look or leaving the wheel requires a neutral right-stick sample before camera input resumes.
4. With neither menu nor wheel, the accepted normal input/radial behavior is retained. Right-stick input is suppressed throughout radial capture. The original outside-menu activation-button overlap behavior and existing bindings are unchanged.

Both checkboxes are under the main menu's **Appearance** button, below the text/centering controls. Settings organization/discoverability is parked in the backlog for a later cleanup.

## Main-menu restoration

The wheel can open over an otherwise idle standard main menu, including when gameplay movement is OFF. It requires the configured enabled XInput radial binding. The main menu is visually suspended while the wheel owns input; its logical open state is never toggled. Closing/canceling the wheel restores its previous tab, scroll, position, size, text/appearance and input preference. A held mouse click cannot become a click on the restored main menu. An explicit main-menu close remains closed rather than being reopened by the overlay.

Right-stick look waits for recentering after wheel release to avoid a camera jump. UI/focus conflicts, disconnect, title/player/session changes, stale polling, or an explicit menu close cancel rather than dispatch a stale selection. The existing one-shot action, identity/authority/build checks and hook retirement remain. Controller-bound hotkeys are held briefly through radial release; keyboard menu-close remains available.

With movement OFF, a verified solo-H2 main-menu pause is suspended only for the wheel and restored after it closes. Existing settings are retained; no forced multiplayer/co-op pause is introduced. Native DirectInput main-menu movement/look/radial remains blocked because its complete neutral mapping is unverified. Xbox and mapped PlayStation pads use XInput; native radial outside the main menu remains as before.

Movement/camera preferences are gameplay input choices and remain excluded from Saved Skins. All current portable preset folders and binding XML files are copied unchanged. Accepted immutable helper switch files are used for the new candidate; live helper state in the working package is left untouched.

## Validation

Release x64 overlay and launcher build. The shared production priority policy passes optional camera on/off, movement on/off, exclusive radial capture, one-shot release, center/recenter gating, UI/focus conflict, disconnect, changed title/player, timeout and explicit-close cancellation. Presentation and pause/hotkey priority matrices pass.

Production WARP rendering passes main-menu/wheel handoff, retained tab/scroll/size/text/options/input flag, held-click isolation, actual camera checkbox off/on/off, explicit-close preservation, and exclusion of camera/movement from aesthetic serialization. The production hook's assembly fixture passes movement-only/camera look/radial camera suppression plus the existing Xbox/native buffer preservation and safe retirement tests. Combined background/font/layout/preset regressions, original captured defaults, radial core/cancellation and verified-solo pause fixtures pass. The actual packaged preset folders are also loaded/rendered through the production code before delivery.

These are automated/offline results. This new camera/radial candidate still needs live verification; no new live acceptance is inferred.

## Explicit live results for the prior combined preview

The user confirmed fonts, animated backgrounds, centering, left-stick movement, the movement toggle (OFF restricts / ON permits movement), hotkeys and the other mods they tested. They also confirmed Saved Skins round-trip: changed appearance and loaded a saved preset, which restored successfully. These statements do not establish every mod/title/route, duplicate/backup/portable-preset case, or held-input/focus/death/title transition. Those remain separately unconfirmed where not explicitly reported.
