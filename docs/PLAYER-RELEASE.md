# Separate player release

The owner authorized packaging and uploading the accepted OBS beta on October 26, 2026, and explicitly authorized committing its release documentation and pushing the accurate source/tag. This supersedes the earlier release hold for this build. Repository visibility remains public; no visibility change is part of the release.

The compiled download has its own [GitHub Release](https://github.com/kyleFFA/halosession/releases/tag/v2026.10.26-obs-beta) and [README download section](../README.md#download-the-compiled-beta). Compiled binaries remain Release assets, separate from source history.

## Complete package

`Session-2026.10.26-obs-beta-win-x64.zip` contains the accepted launcher and overlay, injector/bridge, speedhack, compatible pointer XML, all 1,071 required unchanged physics/helper/static-data files, packaged skins/backgrounds, original notices and the supplementary runtime licenses. Physics is retained. No personal configuration, Saved Skins, films, logs, crash dumps or debug symbols are bundled. First run uses compiled defaults with folder-local persistence.

## Source identity and verification

The product binaries match accepted source commit `f8f06918655847f437db6f5ffcd861e5e0006f7f` after line-ending/whitespace normalization of tracked build inputs. The release tag `v2026.10.26-obs-beta` identifies that product plus the release documentation commit; no native product code is changed for packaging. [The release record](RELEASE-2026.10.26.md) and its JSON manifest provide checksums and actual validation scope.

The runtime review now identifies the official upstream asset and matching helper libraries, and supplies their notices; see [LICENSE-AUDIT.md](LICENSE-AUDIT.md). Preserve those terms with downstream distributions.

The cleaned, extracted package passed static dependency/PE checks and an offline production defaults/artwork fixture. It was not automatically launched, injected or tested on a machine without developer tools. Live acceptance applies to the identical product binaries in the earlier working candidate, as reported by the user for Halo 3, ODST and Reach. Do not describe these checks as a fresh live game or OBS recording test of the cleaned ZIP.

## Known limitation

Halo 3 Invulnerability with H2-style physics can suppress explosive push. The unfinished damage-safe rocket/grenade knockback correction is not included. The supported capture route is OBS Game Capture with Capture third-party overlays disabled; Display/Window Capture are outside its scope. Halo 3's excluded grid can have different bloom/raster appearance. See the release record for setup and controller coverage.
