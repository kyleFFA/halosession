# Independent Speed Overlay — private candidate

**Control > Speed Overlay** adds an initially unbound toggle and independent size, color, opacity and normalized screen position. Default visibility is OFF; default digit height is 4.5% of screen height. These preferences use the existing settings XML and save on normal Session shutdown. Bindings use the existing rebind UI/config. Loading a Saved Skin does not change these separate settings.

The locked overlay uses the existing ImGui/DX11 background screen layer. It remains visible with the main menu closed, creates no input window, and takes no cursor, gameplay-input or pause requests. Existing binding restrictions still apply while the menu, text/rebinding UI or radial owns input. No existing controller mapping or binding is changed.

**Edit overlay position** explicitly enables a drag target and Lock position button. Close the main menu, drag the text, then lock it or press Escape. Only editing requests the cursor and keyboard/mouse guard; controller routing remains unchanged. Held pointer/keyboard input is released before the guard ends. Menu and radial graphics retain priority over the background readout; right-stick ownership is unchanged.

## Data and supported scale

The number reuses the existing physics status API and raw world-units/second value. It also reuses the verified living local-player/build/session checks. Freshness, title/ABI and transition timestamps gate display; stale, dead/missing player, loading, unsupported or invalid contexts show neutral **-- / unavailable**, never a retained stale number. The existing 1.5-second freshness threshold remains. It does not enable/load physics or add a new measurement path. Original H2 and titles without this existing speed telemetry remain unavailable; current eligible readings are the existing H3, ODST and Reach physics/fly telemetry.

Size means **digit height**, up to half the screen height at 1080p and 4K. The physical digit-height bound is 1080 pixels; larger-than-4K outputs clamp there. Long values shrink within a fixed field to fit the screen. Digit changes and the unavailable placeholder keep the same field and unit-label position. Caption bounds and corner anchors are clamped on screen.

Segoe UI remains the typeface. DirectWrite/Direct2D rasterizes just twelve numeric glyphs at the requested physical-pixel scale, using grayscale coverage, then the existing ImGui renderer draws the cached texture. This uses [Microsoft's text rendering APIs](https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-directwrite) and [grayscale antialiasing](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_text_antialias_mode). Giant digits are not enlarged from the menu's small atlas.

Size/DPI changes settle for 0.25 seconds before a background build; physical sizes are grouped in 16-pixel steps. Cold display briefly says Preparing speed font. While resizing, an existing cache displays current values at its crisp available size until the replacement is ready. Ordinary speed, position, color and opacity changes do not rebuild/upload fonts. A hidden overlay cancels pending builds and retains its current GPU cache for quick toggling.

## Cost and verification

Measured dedicated cache sizes, separate from the existing menu atlas:

| Digit height | 1080p | 4K / equivalent 2× DPI |
|---|---:|---:|
| 2.5% | 0.04 MiB | 0.12 MiB |
| 12.5% | 0.56 MiB | 1.89 MiB |
| 50% | 7.35 MiB | 34.19 MiB |

One current cache has a 64 MiB budget. Large builds use additional temporary CPU memory, and old/new GPU textures can overlap during replacement. The final raster pixels are released from CPU memory after upload. The existing menu atlas is unchanged.

The 4K half-screen fixture built glyphs in about 100 ms on its background thread; the one-time render-thread texture upload took about 11–12 ms and can briefly hitch on first show/committed resizing. Steady 4K half-screen rendering measured about 0.10 ms CPU submission and 1.31 ms for the software WARP GPU draw; the small WARP draw was about 0.12 ms. These are local off-screen measurements, not in-game hardware/FPS guarantees. The render fixture writes raw sizes/timings to `out/speed-overlay-validation.json`.

Release x64 native/launcher build passed. Focused production-code tests passed:

- Small/medium/half-screen rendering at 1080p, 4K and 2× DPI; coverage resolution, stable fields, corner/caption bounds, color/opacity and neutral telemetry branches.
- Locked display with no input window/mouse capture; explicit drag/lock, release guard, hidden/minimized handling, coalesced builds and no value-driven font rebuilds.
- Font texture recreation, complete graphics-device replacement and fresh ImGui context reload.
- Actual settings round-trip, invalid-value rejection, new event registration/rebinding and visibility toggle without changing menu state.
- Controller, bound Save/Back and actual menu/radial restoration regressions. Source audit confirms physics, coordinate services, existing hotkeys, input/pause rules, artwork and presets are preserved. Packaged Saved Skins are loaded/rendered before delivery.

Previews are synthetic off-screen fixtures. After live testing this candidate, the user reported everything working flawlessly and authorized committing the source. This is user-reported acceptance, with the scope limits recorded in [live test status](USER-TEST-STATUS.md). Existing on-disk config, bindings and presets were preserved in the tested local package. Working builds and protected helper/core binaries remain intact. This source commit does not publish a compiled player release or promote a default package.
