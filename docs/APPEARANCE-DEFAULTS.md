# Accepted GUI and startup appearance

The user live-tested the seasonal GUI, described its appearance as cohesive and the wheel as working excellently, and accepted it as a working beta. Appearance settings retained changes within the running instance. This is user-reported acceptance, not validation of every game or controller route.

The running instance's appearance values were read without writes, suspension, injection or interruption and checked in repeated identical snapshots. Only sanitized scalar values are included in [appearance-defaults.json](appearance-defaults.json); process details, memory addresses and personal binding files remain outside Git.

Startup uses embedded Session V1, gentle clouds, a 1350x900 frame at (1220,617) on a 3840x2160 reference viewport, and the captured wheel/segment/selection RGBA values. Placement uses the same normalized frame center on other viewports with responsive fitting. Background opacity is explicitly changed from the captured 65% to **50%**. Gentle cloud speed is **1.10 times** the previous speed; x/y travel amplitudes stay 0.007/0.004 of ring diameter. Static remains selectable. Selection, placement and appearance remain per-instance rather than disk-persisted.

## Validation

The Release x64 native overlay and launcher built successfully. The focused production-renderer fixture passed exact float32 palette/alpha comparison, captured placement, responsive fitting at two smaller viewports, actual 50% backing blend over black/white on WARP, 1.10x drift phase, unchanged peak travel and zero movement in static mode. The owner accepted residual regression risk without another game acceptance cycle or broad audit.

Before this defaults-only change, seasonal/loader fixtures passed three skin pairs, large/small rendering, navigation, wheel alignment, missing/corrupt fallback, transaction/refresh and graphics reset handling. Controller hook/core and verified-solo-H2 pause fixtures also passed. Isolated RTX 4090 seasonal rendering at diameter 940/1920x1080 measured about 0.0270-0.0272 ms for the menu and 0.0193-0.0197 ms for the wheel (100 frames after warmup). These are prior isolated draw measurements, not an in-game guarantee or new measurements for this defaults update.

## Delivery boundaries

The updated developer source/assets/docs are authorized for the existing repository. A separate local candidate retains the complete compatible runtime and copies existing bindings unchanged; it does not replace or launch the working/default/rollback package. No compiled public player release, invitation or visibility change is authorized here. Contributor changes/feedback and subsequent owner approval precede release. Optional cloud ripple is a separate follow-up and is absent from this source update.
