# Source and runtime license audit

## Source notices retained

- Root MIT text and copyrights for Maurice Dix and Poorboy are preserved verbatim, alongside `THIRD-PARTY-LICENSES.md`.
- Vendored safetyhook: Boost Software License 1.0; type_safe and ImGuiColorTextEdit: MIT. Their source licenses/copyrights are retained.
- Embedded `debug_assert` retains its full permissive zlib-style notice in the header. It is not silently relabeled as MIT.
- `EnumClass.h` is derived from Gabriel Aubut-Lussier's ArticleEnumClass-v2. Its upstream [BSD-2-Clause license](https://github.com/Dalzhim/ArticleEnumClass-v2/blob/master/LICENSE) is supplied in `licenses/ArticleEnumClass-BSD-2-Clause.txt`; its copyright header is retained.
- Dependencies restored by vcpkg retain their own licenses. The original notice inventory remains included; a future binary release must also collect notices for the exact resolved versions.

## Inherited private-branch comment: publication review item

`HCMInternal/H2CollisionViewer.h` retains an inherited `PRIVATE (collision-viewer-private branch - do not push to public)` comment. Its source counterpart appears in Harc's publicly accessible [CollisionViewer.h at the recorded commit](https://github.com/harctheshark/HaloCheckpointManager---Harcs-Build/blob/240c32c20cbcedd016604c48f769284201db0ade/HCMInternal/CollisionViewer.h), under that repository's [MIT license](https://github.com/harctheshark/HaloCheckpointManager---Harcs-Build/blob/240c32c20cbcedd016604c48f769284201db0ade/LICENSE).

Public MIT evidence supports including this derived source. The inherited development comment alone is not treated as a blanket publication blocker or a new permission requirement. It is retained for transparency. No private research directories or unpublished game binaries are included. The repository owner separately authorized this developer-source upload; a player release remains a separate task.

No concrete missing Harc source permission was established from this audit. If the user wants a courtesy clarification, they could ask: "Your public MIT-licensed commit 240c32c includes CollisionViewer.h with a private-branch comment. Is that comment obsolete, and is it fine to keep the derived collision-viewer source in Session with your license and credits retained?" No contact was made and friendship was not treated as permission. Harc's source authority is separate from Poorboy's physics binary/helper and third-party runtime rights.

## Source assets

The two Session ring/cloud PNGs were generated for this project and are retained byte-for-byte, with a sanitized prompt/provenance record. No Windows fonts are distributed. Old managed-UI game thumbnails, unused DDS/audio media, captured game files and runtime helper binaries are omitted from this developer tree.

## External runtime

The October 26, 2026 release review verified all 1,071 retained h3sb DLL/helper/static-data files byte for byte against Poorboy's official [v1.4.1 asset](https://github.com/PoorboyTTV/PoorboysCheckpointManager/releases/tag/v1.4.1). The archive SHA-256 matches GitHub's asset digest. The license shipped in that asset explicitly includes the Halo 2 physics mod under MIT; its notices are preserved. No concrete conflicting restriction was found for the unchanged Poorboy components. Buildable h3sb implementation source remains absent from this repository.

The helper's separate dependencies were identified and checked against official binary distributions: CPython 3.11.9 embedded Windows runtime (22 matching DLL/PYD files), NumPy 1.26.4 and OpenBLAS (14), Pillow 12.1.1 (7), and PyWin32 308 (2). Their complete supplied notices, including bundled-library notices, accompany the ZIP. Tcl/Tk 8.6.12 notices, libffi terms and PyInstaller's terms/bootloader exception are included. The protected helper remains unchanged; its bootloader version is not recorded by upstream. Microsoft runtime terms reproduced by CPython are retained and called out in the package's runtime notice. Installed native dependency copyright files and version metadata are also collected, alongside source-vendored licenses.

This resolves the previously incomplete dependency-notice inventory for this specific unchanged runtime bundle; it is not permission to substitute unreviewed binaries or extracted game payloads. The review found no separately distributed game executable/DLL payload in the retained static manifest. Personal recordings, generated world/memory captures and development files are excluded. The owner authorized compiled upload separately from source publication. See [the release verification record](RELEASE-2026.10.26.md).
