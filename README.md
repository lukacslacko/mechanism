# Mechanism

An interactive Unreal Engine material and mechanism study: a square steel plate
on a walnut table in a bright white hall, with a drilled square brass bar turning
on a central steel pivot. This is the starting point for a mechanical computing
machine simulator.

![Actual standalone Unreal render](docs/images/detail.png)

The pointer rotates once per minute. A real through-hole clears the pin, and a
steel collar supports the bar above the plate. Lighting and reflections use
real-time Lumen. The motion is prescribed animation; a mechanical solver and
VR interaction are future work.

## Start on Windows 11

1. Install **Unreal Engine 5.8.4** through Epic Games Launcher, including Windows
   platform support. These assets were saved in 5.8; do not open them in an older
   engine. Blender is not required to run or build the checked-in scene.
2. Install Visual Studio and the C++/Windows SDK components described in
   [the Windows guide](docs/WINDOWS.md#prerequisites).
3. Install Git for Windows. All project assets are included;
   no Git LFS, paid asset pack, or external plugin download is required.
4. From PowerShell in the folder where you keep projects:

```powershell
git clone https://github.com/lukacslacko/mechanism.git
cd mechanism
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Editor
```

Then open `MaterialHall.uproject`, wait for shaders, and choose **Play → Standalone
Game**. To work on C++, generate project files from the `.uproject` context menu
and use **Development Editor / Win64** in Visual Studio.

To build a regular standalone executable:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Package
.\Saved\StagedBuilds\Windows\MaterialHall.exe
```

If the engine is elsewhere, append `-EnginePath 'D:\Epic\UE_5.8'` to the build
command, or set `$env:UE_ROOT` to that directory. See the [Windows guide](docs/WINDOWS.md)
for RTX 5070 hardware ray tracing, packaging, testing, and troubleshooting.

## Controls

| Input | Action |
| --- | --- |
| W / S | Move forward/back in the direction you are looking |
| A / D | Strafe left/right |
| Q / E | Move down/up in world space |
| I / K, J / L | Look up/down, left/right |
| Shift | Move three times faster |
| Right-drag / scroll | Orbit / zoom |
| 1 / 2 / 3 / 4 | Table / detail / hall / pivot close-up |
| Space | Pause/resume rotation |
| R | Reset pointer and camera; resume rotation |
| F | Cycle Smooth / Balanced / Fine |
| Escape | Quit the game |

The UI also has camera and quality buttons and a shared metal roughness slider.
Balanced starts at 65% internal resolution with TSR reconstruction and a 60 fps
limit; Fine uses 85%. Some temporal grain remains in soft shadows and reflections.

## Project layout

- `MaterialHall.uproject`: open this in Unreal; game module name remains `MaterialHall`.
- `Source/MaterialHall/`: native C++ interaction, Slate UI, rotation and runtime tests.
- `Content/MaterialHall/WhiteHall.umap`: complete editable hall and assembly.
- `Content/MaterialHall/Assets/` and `Materials/`: editable imported meshes, textures and material graphs.
- `SourceAssets/`: original FBX meshes and texture maps for reimporting.
- `Tools/`: Windows/Mac build scripts, Blender mesh generators and Unreal Python helpers.
- `docs/ASSETS.md`: dimensions, geometry editing and regeneration.
- `docs/validation/`: recorded verification from the tested Mac build.

Unreal Editor's Python plugins are used only by asset tools. The packaged app
runs compiled C++, and does not need Python, Blender, Xcode or Visual Studio.

## macOS

Install UE 5.8.4 and a compatible full Xcode with its Metal toolchain. Run:

```bash
./Tools/build-mac.sh Package
```

`UE_ROOT` and `DEVELOPER_DIR` can select nonstandard installations. The complete
app is `Saved/StagedBuilds/Mac/MaterialHall.app`; copy that entire bundle.
The script builds and validates in the background without opening the editor.

## Validation status

This source snapshot was rebuilt from its clean repository folder, packaged,
rendered and tested on an Apple M2 Pro with macOS 27.0.1 and UE 5.8.4. Its checks covered a complete revolution,
plate/table and collar/pointer contact, fixed pivot alignment, pause/resume,
reset and 13 injected keyboard navigation tests. See the recorded JSON reports
and [pivot close-up](docs/images/pivot.png).

The Windows build script and DX12/SM6 configuration are provided for the next
machine; **Windows, RTX hardware ray tracing and Quest have not been runtime-tested**.
This version has no VR pawn, OpenXR configuration or Quest controller interactions.

## License

Original project work is [MIT licensed](LICENSE). The bundled wood textures are
CC0, with their authors and source documented in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
Unreal Engine is installed separately under Epic's license and is not included.
