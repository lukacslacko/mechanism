# Third-party materials

The repository's original C++, Python, build scripts, documentation, generated
meshes, material graphs and scene composition are provided under [MIT](LICENSE).

## Wood textures — CC0 1.0

[Wood Table 001](https://polyhaven.com/a/wood_table_001), from Poly Haven.
Photography: **Dimitrios Savva**. Processing: **Rico Cilliers**.

The following are the supplied 2K source maps and their Unreal imports:

| Source | Unreal asset |
| --- | --- |
| `SourceAssets/Textures/wood-color.jpg` | `Content/MaterialHall/Assets/WalnutColor.uasset` |
| `SourceAssets/Textures/wood-normal.jpg` | `Content/MaterialHall/Assets/WalnutNormal.uasset` |
| `SourceAssets/Textures/wood-roughness.jpg` | `Content/MaterialHall/Assets/WalnutRoughness.uasset` |

They retain [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/), as
specified by [Poly Haven's asset license](https://polyhaven.com/license).
The normal map is OpenGL convention; its Unreal import flips the green channel.

## Unreal Engine — external dependency

Unreal Engine, its bundled tools, plugins, built-in meshes and fonts are installed
separately under [Epic's Unreal Engine license](https://www.unrealengine.com/eula/unreal).
They are not relicensed by this repository's MIT license. Engine source and
compiled engine binaries are not included here. The map references built-in
engine primitives; Unreal supplies those when it opens or cooks the project.

`Content/MaterialHall/Materials/VRWidget.uasset` is derived from Unreal's
`/Engine/EngineMaterials/Widget3DPassThrough` with inverse eye adaptation added
for readability. This engine-derived material is subject to Epic's Unreal
Engine license, not the repository's MIT license. The generation script is
`Tools/build_vr_ui.py`.
