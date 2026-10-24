# Combined private preview

> Historical candidate report. This implementation is included in the user-accepted source; see [current acceptance status](USER-TEST-STATUS.md). Statements below about pending acceptance or commit authorization describe the earlier candidate stage.

One candidate combines four selectable center backgrounds, text sizing, centering, movement with the main menu, and Saved Skins. Source is based on `2e04736`; all changes for this candidate remain local. No commit, push, player release, launch, injection, default promotion or rollback replacement was performed.

## Appearance

Appearance > Center background selects Original clouds, Bright anime sky, Pastel anime clouds, Quiet twinkle stars, or Retro sparkle stars. The two approved source PNGs are byte-identical copies. Clouds reproduce the approved 96-frame back-and-forth crop at quarter speed: 32 seconds per loop. Stars retain the exact seeded fields, twinkle/sparkle formulas and 8-second timing. This is the approved back-and-forth animation, not a claim of seamless continuous scrolling. The center disk uses the actual skin opening and stays behind the fixed ring/text/controls. Animation stops while neither menu nor wheel is drawn; static mode uses the initial frame. Speed is adjustable relative to those approved timings. No GIF decoding or texture uploads occur each frame.

Center opacity is independent of the existing 50% backing. New center art starts fully opaque; both are adjustable. Automatic text contrast uses light text/dark controls over stars and the existing dark text/light controls over sky/clouds. Text size ranges from 0.75x to 1.60x, including wheel and navigation labels. Center menu options starts on: short groups are centered on the opening; long pages and the Appearance popup scroll. No rectangular main-panel background is added.

## Main-menu movement

The option is OFF initially and is intentionally excluded from aesthetic presets. When enabled, only the selected Xbox/PlayStation **XInput** controller's left stick passes, after a verified living local player, foreground focus and a neutral/released input sample. Camera, buttons and triggers are suppressed; keyboard and mouse gameplay remain blocked. UI text/modal conflicts, unavailable/unknown routes, death/loading/session changes and stale render heartbeat prevent movement. Native DirectInput movement remains fully blocked because trigger-neutral mapping is unverified; existing radial native support is retained. Existing additional full-input requests still take precedence.

Movement mode disables main-menu pause; it does not force pause in multiplayer or co-op. With movement OFF, the accepted verified-solo-H2 pause rule remains. Closing retains keyboard/mouse blocking until held UI keys/clicks release and keeps controller actions/camera suppressed until their release sample, avoiding click-through or a camera jump. Controller/mouse-bound tool hotkeys are blocked while the main menu is open. Existing keyboard/menu bindings are copied without modification.

## Saved Skins

Save new creates `SavedSkins/<name>/preset.xml` plus portable PNG assets when needed. It captures ring/cloud/node/plaque selection, center art and animation/static mode/speed, colors/alpha, backing/center opacity, menu dimensions/position/reference viewport, text size/color/contrast and centering. Position adapts from the saved frame center to the current viewport; size fits the available screen. It omits gameplay options and private hotkeys. Copy the entire preset folder to another compatible candidate's SavedSkins folder. Load validates version, finite ranges, contained paths, file/pixel budgets, PNGs and ring geometry before committing; invalid loads retain the complete current appearance. UI loads commit at the next frame boundary. Explicit Replace keeps the previous folder as a loadable backup. Existing preset names are never silently overwritten. Four portable starter folders are supplied.

## Test bench

Release x64 native overlay and launcher build successfully. Focused WARP tests passed four static/animated backgrounds, approved 32/8-second timing, hidden-time freezing, 24 size/text renders, actual-pixel containment inside the opening, menu/wheel geometry, centered short groups, long-page scrolling, actual movement/centering/text-size controls, and the Appearance/Saved Skins panel at 720p with the largest text setting. Preset round-trip, embedded/external assets, duplicate/unsafe-name rejection, corrupt PNG/NaN transaction failure, explicit replacement backup, preset refresh, and graphics reset passed. The four actual packaged starter presets also loaded and rendered through production code. The captured original defaults/50% backing/1.10x drift check still passes.

Movement policy tests cover enabled/disabled, focus/UI/unready fallback, neutral/release gates, key/click release blocking, hotkey guards, and filtering while preserving only left movement. The production native-hook assembly fixture passed XInput/native buffers, movement-only filtering, consumer progression, restoration and in-flight retirement. Radial core/cancellation and verified-solo pause regressions passed. All three seasonal skins and generic loading/validation/fallback/refresh/reset regressions passed. Live movement/game acceptance remains for the checklist; no automatic test touched the running game.

## Performance

Off-screen NVIDIA GeForce RTX 4090, 1920x1080, diameter 940, 100 measured frames per case after 15 warmups; 2,000 timestamped frames across original/four backgrounds, static/animated, menu/wheel. These isolated draws are not an in-game FPS promise. Image decoding/selection and file save/load are excluded.

| Animated menu | CPU median | GPU median | Vertices | Draw commands |
| --- | ---: | ---: | ---: | ---: |
| Original | 0.0521 ms | 0.0215 ms | 4504 | 9 |
| Bright sky | 0.0559 ms | 0.0225 ms | 4633 | 10 |
| Pastel clouds | 0.0560 ms | 0.0236 ms | 4633 | 10 |
| Quiet twinkle | 0.0647 ms | 0.0256 ms | 5568 | 10 |
| Retro sparkle | 0.0871 ms | 0.0297 ms | 7236 | 10 |

Raw static/wheel results are in OFFLINE-PERFORMANCE.json. Textures are immutable and loaded only on selection or restore. Star animation uses small procedural draw geometry; no full-screen blur or new render target/shader is used.
