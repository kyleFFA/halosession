# OBS capture routing

The accepted source adds the Control page's Bypass OBS Capture checkbox and status display. Compatibility is discovered from the loaded OBS hook and its supported mapping rather than a fixed module address. Direct and canonical chained detours are validated before enabling the route; unsupported or unavailable routes report their status and retain local rendering. Graphics-thread ownership and callback draining govern changes and cleanup.

The supported configuration is OBS Game Capture with Capture third-party overlays disabled. Display Capture, Window Capture and unrelated game-render effects are outside this route. Session cannot inspect the resulting recording, so status alone is not recording verification.

Halo 3 normally draws its grid in an earlier engine pass. A fresh post-capture routing lease and successfully prepared frame renderer allow that early draw to stand aside. Per-Present accounting prevents a duplicate grid or suppression based on stale engine history. An already drawn early grid produces a transition warning until a clean frame. Unavailable preparation or an expired route retains the local engine path. ODST and Reach retain their frame paths without new engine hooks.

During verified exclusion, Halo 3's grid uses the post-bloom frame renderer. Its glow and raster coverage can differ from the engine pass. Structure/instance visibility, colors, widths, opacity, wire view, No Bloom and X-ray settings are preserved. The late path retains scene depth, gun/arms occlusion and player masks; depth copies refresh across title, viewport and device changes.

The accepted package also uses folder-local settings. Shared settings fallback/mirroring is disabled; ordinary local serializers remain in use. Runtime binaries, personal configurations, logs, recordings and diagnostic memory/disassembly files are excluded from this source commit. Physics integration and invulnerability behavior are unchanged.

## Validation and acceptance

Release x64 native overlay and launcher builds passed using the pinned local dependency cache. Focused fixtures passed for direct/chained detour discovery and 200 synthetic post-capture frames, unsupported routes and cleanup, Control checkbox/status rendering, 1,000 mixed grid-routing frames, and production WARP depth/occlusion, no-pixel preparation, X-ray distinction and graphics-state restoration. Existing Speed Overlay and menu/radial restoration checks also passed.

The user reported successful live OBS exclusion in Halo 3, ODST and Reach, then reported that everything was working flawlessly and authorized this commit on October 25, 2026. These are user-reported results, separate from the offline fixtures; they do not establish exhaustive coverage of all recordings, transitions or hardware. This commit does not publish or promote a player package.
