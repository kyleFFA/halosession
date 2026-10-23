# Source and runtime license audit

## Source notices retained

- Root MIT text and copyrights for Maurice Dix and Poorboy are preserved verbatim, alongside `THIRD-PARTY-LICENSES.md`.
- Vendored safetyhook: Boost Software License 1.0; type_safe and ImGuiColorTextEdit: MIT. Their source licenses/copyrights are retained.
- Embedded `debug_assert` retains its full permissive zlib-style notice in the header. It is not silently relabeled as MIT.
- `EnumClass.h` is derived from Gabriel Aubut-Lussier's ArticleEnumClass-v2. Its upstream [BSD-2-Clause license](https://github.com/Dalzhim/ArticleEnumClass-v2/blob/master/LICENSE) is supplied in `licenses/ArticleEnumClass-BSD-2-Clause.txt`; its copyright header is retained.
- Dependencies restored by vcpkg retain their own licenses. The original notice inventory remains included; a future binary release must also collect notices for the exact resolved versions.

## Inherited private-branch comment: publication review item

`HCMInternal/H2CollisionViewer.h` retains an inherited `PRIVATE (collision-viewer-private branch - do not push to public)` comment. Its source counterpart appears in Harc's publicly accessible [CollisionViewer.h at the recorded commit](https://github.com/harctheshark/HaloCheckpointManager---Harcs-Build/blob/240c32c20cbcedd016604c48f769284201db0ade/HCMInternal/CollisionViewer.h), under that repository's [MIT license](https://github.com/harctheshark/HaloCheckpointManager---Harcs-Build/blob/240c32c20cbcedd016604c48f769284201db0ade/LICENSE).

Public MIT evidence supports including this derived source. The inherited development comment alone is not treated as a blanket publication blocker or a new permission requirement. It is retained for transparency. No private research directories or unpublished game binaries are included. The current task prepares local review and does not authorize publication.

No concrete missing Harc source permission was established from this audit. If the user wants a courtesy clarification, they could ask: "Your public MIT-licensed commit 240c32c includes CollisionViewer.h with a private-branch comment. Is that comment obsolete, and is it fine to keep the derived collision-viewer source in Session with your license and credits retained?" No contact was made and friendship was not treated as permission. Harc's source authority is separate from Poorboy's physics binary/helper and third-party runtime rights.

## Source assets

The two Session ring/cloud PNGs were generated for this project and are retained byte-for-byte, with a sanitized prompt/provenance record. No Windows fonts are distributed. Old managed-UI game thumbnails, unused DDS/audio media, captured game files and runtime helper binaries are omitted from this developer tree.

## External runtime

The protected local DLLs are not changed or uploaded. The retained Poorboy copyright line explicitly includes the Halo 2 physics mod, which is relevant permission evidence for Poorboy's work. The h3sb implementation/packed helper is not present as buildable source here; the full component inventory and terms for all helper dependencies have not been established from that MIT text alone. This is an unresolved release-dependency review, **not a finding that distribution is forbidden**. See [the player release plan](PLAYER-RELEASE.md) for a path to a complete usable download.
