# Windows 11 development and packaging

## Prerequisites

Use Unreal Engine **5.8.4** from Epic Games Launcher, with Windows target support,
plus Git for Windows and a current NVIDIA driver on the RTX 5070 machine.

For the compiler, use a current **Visual Studio 2026** installation with MSVC
14.50, or **Visual Studio 2022 17.14** with MSVC 14.44. Include **Desktop development
with C++**, **Game development with C++**, and a Windows 11 SDK (10.0.22621.0 or
newer; 10.0.26100 or newer is Epic's current recommendation). Use the
[official Unreal/Visual Studio setup guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)
for the remaining installer components.

The installed UE 5.8.4 toolchain configuration rejects early buggy builds of
these compiler families. Use MSVC **14.50.35723 or later within 14.50**, or
**14.44.35211 or later within 14.44**. This detail comes from
`Engine/Config/Windows/Windows_SDK.json` in the tested 5.8.4 engine installation.
If UnrealBuildTool reports a banned compiler, add the matching current component
through Visual Studio Installer rather than disabling the check.

## Clone and open

Choose a short local path outside OneDrive, for example `C:\dev\mechanism`.

```powershell
git clone https://github.com/lukacslacko/mechanism.git C:\dev\mechanism
cd C:\dev\mechanism
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Editor
Invoke-Item .\MaterialHall.uproject
```

The build script uses `C:\Program Files\Epic Games\UE_5.8` by default. Override
with `-EnginePath 'D:\Epic\UE_5.8'` or `$env:UE_ROOT`. It does not open the editor
automatically. First launch recompiles platform-specific shaders, which can take
several minutes. The default map is `/Game/MaterialHall/WhiteHall`.

Use **Play → Standalone Game** to see the native UI and input controls. The
ordinary editor viewport shows the saved scene but does not run the pointer's
C++ animation. Close Unreal before rebuilding changed C++ headers if Live Coding
holds a module lock.

## Package an EXE

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Package
.\Saved\StagedBuilds\Windows\MaterialHall.exe
```

The script builds the editor module, builds the game, cooks the map, packages
content and stages a **Development** build. Distribute the entire
`Saved\StagedBuilds\Windows` folder, including its `Engine` and `MaterialHall`
subdirectories. The launcher EXE alone is not self-contained.

`-prereqs` asks Unreal to stage its prerequisites installer. If another PC lacks
the Visual C++ runtime, run the included installer under
`Engine\Extras\Redist\en-us` when present, or install the current Microsoft
Visual C++ Redistributable. A packaged game does not need Unreal Editor or an IDE.

## RTX 5070 rendering

The repository explicitly targets **DirectX 12 / Shader Model 6** on Windows.
Its initial Lumen settings preserve the software-traced Mac baseline, so first
confirm the scene works before switching the rendering path.

To try the RTX hardware path, open **Project Settings → Engine → Rendering**:

1. Enable **Support Hardware Ray Tracing** and **Support Compute Skin Cache**.
2. Enable **Use Hardware Ray Tracing when available** for Lumen.
3. Restart the editor and allow shader compilation to finish.
4. For higher-quality reflections, try **Ray Lighting Mode → Hit Lighting for
   Reflections**, then compare frame time with Surface Cache lighting.
5. Repackage after changing these settings. Runtime console switches cannot add
   ray-tracing shader permutations to a build that was cooked without support.

Hit Lighting evaluates more work per reflection ray. Treat it as a quality
experiment, not a promised frame-rate improvement. See Epic's
[hardware ray tracing documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-ray-tracing-in-unreal-engine).
The UI's Fine setting increases internal resolution and quality; it does not
enable hardware ray tracing. There is no DLSS plugin dependency in this project.

## Verify on the new machine

Saved scene geometry checks, without opening the editor UI:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Validate
```

After packaging, an approximately 105-second offscreen runtime test:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build-windows.ps1 -Action Test
```

The test captures table/detail/hall/pivot views and checks the full rotation,
support heights, alignment, Space pause/resume, R reset and navigation keys.
The script reports failure if the runtime JSON does not say `passed: true`.
Each run writes to a new `Saved\Validation\Run-...` directory, so an old result
cannot be mistaken for a new pass. Inspect the PNGs as well as the numbers.

Windows compilation and rendering have not yet been executed for this source
release. The recorded reports in `docs/validation` are from the Mac build.

## Common issues

- **Missing or incompatible module:** use the matching 5.8 engine, install the
  compiler components, then run the Editor build again. Generated `Binaries`
  and `Intermediate` folders can be regenerated; do not remove `Content`.
- **Wrong map or no controls:** open `WhiteHall` and use Play → Standalone Game.
- **Camera keys stop responding:** click the game viewport to return keyboard
  focus; Q/E change height, and F changes quality.
- **Slow first launch:** wait for shader compilation; try Smooth for interactive
  work, then compare Balanced/Fine after shaders finish.
- **Want to use Quest 2 over USB:** the present project is a desktop viewer.
  A PC VR/OpenXR pawn, stereo rendering configuration and controller interaction
  still need implementation. Installing Android packaging alone will not add them.
