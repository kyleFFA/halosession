# Skin loader and seasonal artwork

Appearance offers Session V1, Garden, Aurora and Autumn, with Refresh skins. The user accepted the seasonal GUI and wheel as a working beta. Current startup appearance and validation are documented in [APPEARANCE-DEFAULTS.md](APPEARANCE-DEFAULTS.md).

A versioned manifest replaces ring/cloud artwork and optionally node/plaque artwork while preserving actual labels, controls, bindings and action logic. Shared measured opening metadata aligns content and the circular wheel. Selection commits at a frame boundary, after validation; missing/corrupt/invalid assets restore the complete embedded V1. Graphics resets retain a valid selection. Skin and appearance changes remain per-instance.

Each seasonal folder includes separate transparent ring/cloud PNGs, its manifest and 360-sample opening calibration. Assets retain the original generated PNG bytes, with no pixel/alpha postprocessing. [Exact prompts](../skins/SEASONAL-PROMPTS.txt) record the approved faint direction. A small fixed cloud mesh increases outer opacity for those skins; embedded V1 retains its uniform path. No blur, additional shader, render target or per-frame texture upload is used.

See [authoring instructions and limits](../skins/README.md), [the manifest template](../skins/_template/skin.xml.example), and [V1 generation history](../HCMInternal/Assets/RINGWORLD-PROMPTS.txt). Native test programs can generate local ignored previews; screenshots and compiled artifacts are not stored in this source repository.
