# Editing the scene and geometry

The checked-in `.umap` and `.uasset` files are Unreal source assets. Open them in
the editor to change materials, lights, camera composition or hall geometry.
No generator needs to run before the first build.

All geometry uses **centimetres**, with **Z up**. The tabletop is at Z = 82 cm.

| Part | Dimensions / position |
| --- | --- |
| Steel plate | 100 × 100 × 1.6 cm, bottom Z = 82 |
| Steel pin | Diameter 3 cm, height 9.8 cm, bottom at plate top |
| Support collar | Outer diameter 6.4 cm, bore diameter 3 cm, height 0.8 cm |
| Brass pointer | 80 × 7.4 × 7.4 cm; center bore diameter 3.4 cm |
| Pointer support | Bottom on collar top; 0.8 cm clearance over plate |
| Pin projection | 1.6 cm above the brass |

The pointer mesh origin is the center of its bore. `MaterialHall.cpp` rotates
it around the world Z axis at 6 degrees/second. The animation changes yaw only,
so it cannot progressively translate off the pivot. It is kinematic; no torque,
friction, bearing or collision response is simulated yet.

## Regenerate meshes

The mesh scripts were run with Blender 5.1. They create bevelled meshes and export
FBX files directly into `SourceAssets/`:

```powershell
blender --background --python .\Tools\build_meshes.py
blender --background --python .\Tools\build_pivot_meshes.py
```

Add Blender to PATH or use its full executable path. The first script generates
the table parts. The second generates the plate, pin, collar and brass bar,
including actual Boolean through-holes; it checks watertight surfaces and ray
clearance through the bores. Its report is `Saved/Validation/mesh-check.json`.

After the pivot FBXs change, run `Tools/update_pivot_scene.py` from Unreal's
**Tools → Execute Python Script**, or with the editor closed:

```powershell
$Engine = 'C:\Program Files\Epic Games\UE_5.8'
& "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$PWD\MaterialHall.uproject" -run=pythonscript "-script=$PWD\Tools\update_pivot_scene.py" -nullrhi -unattended -nosplash
```

This script reimports the four pivot assets and **replaces the assembly actors in
WhiteHall**, then saves and validates it. Commit your edits before running it if
you want to preserve manually adjusted assembly positions. Other hall actors are
retained. Reimport table FBXs from their assets' context menus when needed.

## Textures and materials

The source texture maps are in `SourceAssets/Textures`. Unreal imports use
project-relative source paths. `Tools/reimport_textures.py` imports the
three bundled maps through the legacy texture importer and sets the normal-map
convention. Run it with the same commandlet pattern after editing those maps.
You can also use the texture asset context menu to reimport individual maps.

The material names `Brass` and `BrushedSteel` identify the metals for the runtime
roughness slider. Mesh names `BrassPointer`, `SteelPlate`, `PivotPin` and
`PivotCollar` identify the assembly. If you rename these assets, update the C++
lookup and scene validation script together.

Default metal roughness is 0.24. The slider spans 0.08 to 0.55. A separate
roughness wood map is included for experimentation; the current walnut graph
uses a constant roughness value of 0.29.

See [third-party notices](../THIRD_PARTY_NOTICES.md) for texture attribution.

## AND linkage

The default runtime demo uses 11 original parts in `SourceAssets/AndGate` and
their imported meshes and seven finishes in `Content/MaterialHall/AndGate`.
Regenerate with `blender --background --python .\Tools\build_and_meshes.py`, then
run `Tools/import_and_assets.py` through the Python commandlet shown above.
The generation script checks manifold surfaces and positive volume. The import
script replaces the gate meshes without changing the saved hall or pointer.
`AHallAndGate` assembles the guides, links, sliders and layered pin joints at
runtime. See [AND-GATE.md](AND-GATE.md) for the exact linkage dimensions.
