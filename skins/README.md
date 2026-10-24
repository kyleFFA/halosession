# Skin authoring, version 1

Skins replace artwork while retaining the four navigation buttons, real text, form controls, radial action logic and bindings. The default Session V1 artwork remains embedded as a fallback. Garden, Aurora and Autumn from the approved faint direction are supplied; concept sheets are not runtime textures.

At runtime, put each authored skin in its own folder under **`skins` beside `PCMInternal.dll`**. Use an ASCII folder ID such as `garlands-clouds`; the display name may contain spaces. Appearance → Skin → Artwork lists valid manifests. **Refresh skins** rescans folders and reloads the active external skin at the next frame boundary. Failed loads restore Session V1. Selection and appearance last for the loaded Session instance, including graphics resets; they are not saved between launches in this preview.

## Files

Use the nonselectable [manifest template](_template/skin.xml.example). Rename it to `skin.xml` only after supplying finished assets and calibrated opening values:

```xml
<SessionSkin version="1" name="Garlands and clouds">
  <Textures ring="ring.png" clouds="clouds.png"
            node="node.png" plaque="plaque.png" />
  <Opening centerX="0.5" centerY="0.5" radius="0.32"
           wheelInsetRatio="0.07" />
</SessionSkin>
```

The values above are illustrative; measure the actual artwork.

| Texture | Required | Role |
| --- | --- | --- |
| `ring` | Yes | Square foreground artwork: ringworld, wreath, garland or other frame. Empty transparent opening and transparent corners. |
| `clouds` | Yes | Transparent cloud/backdrop layer. Static or the existing gentle two-layer movement. Transparent corners prevent a rectangular background. |
| `node` | No | Circular navigation-button artwork, drawn in the existing square node bounds. Use a square PNG with suitable alpha; text remains real. |
| `plaque` | No | Horizontal navigation label backing, reused on both sides. A symmetric, text-free PNG works best. Real labels and a steady text backing remain above it. |

Omit optional attributes to retain the familiar vector button artwork. Other form widgets stay real ImGui controls; this first skin format does not replace their implementation or bake their labels into artwork. Wheel colors and opacity remain independent Appearance controls.

A cloudless theme can supply a small, fully transparent PNG for the required cloud slot.

Optional `<Clouds rimOpacity="0.96" />` adjusts outer cloud alpha with a small fixed mesh while retaining the quiet center. Omit it for the original V1 uniform rendering.

## Alignment

Measure the **inner opening**, excluding outer clouds and the canvas boundary. `centerX`/`centerY` are the measured center divided by image width/height. `radius` is the inner circle radius divided by the square image width. `wheelInsetRatio` is the fraction of that radius left as margin: wheel radius = opening radius × (1 − inset ratio).

The same metadata positions the circular wheel, center badge and central content groups, scales the inscribed scroll area, and supplies the interior hit region. The navigation arrangement and controller sector directions remain familiar. Metadata is validated, but the loader cannot infer whether an arbitrary garland drawing was measured correctly: inspect an offline render before calling it production-ready. The V1 cyan-edge measuring script is specific to that artwork, not a general detector for flowers or other themes.

## Limits and fallback

- Manifest version 1, at most 64 KiB; up to 64 discovered skins.
- Static PNG files, each at most 16 MiB and 2048×2048 pixels; ring must be square.
- Combined decoded skin textures: at most 8,000,000 pixels (32 MB of RGBA data). Fonts and transient/in-flight resources are separate.
- Opening radius 0.15–0.44; inset ratio 0.03–0.25; the full opening must fit inside the image with a small border.
- Required ring/cloud images must have transparent corners. Nine samples near the opening center must be clear in the ring image.
- Asset paths must remain within the skin folder. Absolute paths, parent traversal and escaping links are rejected.

Loading is transactional. Missing/corrupt images, invalid geometry or exceeded limits restore the complete embedded V1 skin instead of mixing old and new layers. Files are decoded synchronously only when a skin is selected, refreshed or restored after a graphics reset; that operation can pause the UI frame. No file polling or decoding occurs during ordinary frames. There is no executable skin code, shader or font loading.

## Offline review

After building the existing sketch fixture with `VSDIR` configured, run from this source root:

```powershell
cmd /c tools\test-sketch-ui.cmd
out\sketch_ui_test.exe "C:\path\to\authored-skins" "garlands-clouds" "out\garlands-preview.png"
```

This exports an offline production-renderer PNG with representative controls. It does not start Session or connect to MCC. A failed skin load produces the fallback image and a nonzero exit code. Check the exit code, balanced wheel inset, content centering, readable labels, node spacing and scrolling before accepting an asset set.

The ignored `out/skin-fixtures` files belong to loader tests. They intentionally reuse mismatched artwork to exercise replacement and failure paths; they are **not production skins** and are not copied to the runtime skin directory.

## Saved aesthetic presets in the combined preview

Appearance > Saved Skins saves complete aesthetic folders, including copied artwork, under SavedSkins beside the DLL. This is separate from the authored skin format above and excludes gameplay/hotkeys. Copy the whole preset folder for portability; load/replace behavior and validation are in [COMBINED-PREVIEW.md](../docs/COMBINED-PREVIEW.md).
