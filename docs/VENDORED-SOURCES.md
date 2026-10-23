# Vendored source provenance

These directories contain ordinary source files with their original licenses. No nested Git repositories or missing Git submodules are uploaded.

| Component | Upstream revision | Local changes |
|---|---|---|
| HCMInternal safetyhook, Burnt-o fork | `8fd3c9e252b9e4760ecf81ef2e30853f0f6a2fc9` | Existing Session accessor additions in `inline_hook.hpp` and `mid_hook.hpp` for safe hook retirement; retained from the tested baseline. |
| HCMSpeedhack safetyhook, Burnt-o fork | `a302cb67a69f2899010e956035cb823a7331522c` | None relative to the supplied upstream checkout. |
| type_safe | `cdf334cd8e5fcb5e21ab470decdfcbd190ef7347` | None; embedded debug_assert notice retained. |
| goossens/ImGuiColorTextEdit | `f6922e18af7d1a3147d98f498f7c6b0515bc7468` | None; inactive upstream editor source/header retained. |
| Boost regex, editor's nested source | `4cbcd3078e6ae10d05124379623a1bf03fcb9350` | Retained source with Boost license/copyright notices. |

Upstream documentation/examples/tests not required by the selected native build are omitted. Boost headers used by retained code come through the pinned vcpkg manifest; the retained nested editor dependency is source only. Compile/include source required by the native project is retained.

Upstream links: https://github.com/Burnt-o/safetyhook, https://github.com/foonathan/type_safe, https://github.com/goossens/ImGuiColorTextEdit.

Do not replace the internal safetyhook tree with an unpatched checkout: the tested shutdown code relies on its accessor additions. New vendor changes should be reviewed and documented separately.
