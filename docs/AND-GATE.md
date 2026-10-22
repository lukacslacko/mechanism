# Rod-based AND gate

Two parallel input sliders are 36 cm apart. The output slider runs parallel to
them on the centre line. Each input has a 34.2 cm stroke. Two equal 35.501549 cm
links meet at a floating pin; a 33.634284 cm output link connects that pin to the
output slider. All dimensions are pivot-centre distances.

| A | B | Output C |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 1 |

At equal input positions, the two input links form a symmetric V and the entire
linkage translates. With unequal inputs the floating pin swings sideways; the
output link rotates, compensating for the floating pin's forward travel.
The output returns to exactly the same zero position at either mixed endpoint.
During a single-input transition, its largest temporary displacement is about
2.52 cm, or 7.36% of the full stroke. The mechanism retains this physical motion.

## Geometry

`HallAndKinematics.h` solves the intersection of two equal-radius circles centred
on the input pivots, selecting the branch toward the output. It then intersects
the output link's circle with the output axis, selecting the right-hand branch.
No truth-table value is used to position the output.

Let half the input spacing be `a = 18`, the stroke `s = 34.2`, and the resting
forward reach of the V be `h = 30.6`. The equal input-link length is
`L = sqrt(a² + h²)`. In a mixed state:

```text
d = sqrt(4a² + s²)
t = sqrt(L² - d²/4)
x = 2at/d
y = st/d
k = s/2 + x - h
M = (y² + k²) / (2k)
```

Choosing the output-link length `M` this way makes
`sqrt(M² - y²) = M - k`, exactly cancelling the mixed-state forward movement.
The solver remains on the same nonsingular branch across the full input square.

The inputs follow critically damped motion, including reversals during travel.
The output is solved anew from their positions every frame. This is a kinematic
demonstration; it does not simulate loads, friction, collisions or signal latching.
The three links sit at different heights on common vertical pins with spacers,
so their planar lengths remain constant without overlapping their bodies.

## Use and validation

Click the cyan or orange handle, use the panel's input buttons or truth-table
states, or press **Z / X**. **Space** cycles 00 → 10 → 11 → 01, with time to settle
at each state. **R** returns both inputs to zero. In Quest Link, right-controller
**A / B** toggle inputs, and the trigger selects handles or panel buttons.

After packaging, run `Tools/build-windows.ps1 -Action GateTest`. It writes
`and-check.json` and renders to a fresh `Saved/Validation/Run-*` folder. The report
checks 11 transitions, the keyboard path, visible-link endpoint alignment,
single-input drift, handle collision/selection and demo visibility switching.
`-Action VRTest` additionally checks the simulated Quest control path and panel.

Unreal automation `MaterialHall.Mechanism.AndLinkage` covers all 10,201 points
on a 101 × 101 input grid and motion at 30, 60, 90 and 120 fps. These tests verify
the geometry and controls; physical headset viewing requires a separate session.

## Original assets

`Tools/build_and_meshes.py` builds 11 original beveled parts in Blender, including
real bores in the links, sliders and guides. Source FBXs are checked into
`SourceAssets/AndGate`. `Tools/import_and_assets.py` imports those meshes and
creates seven metal/enamel finishes in `Content/MaterialHall/AndGate`.

Run the Blender script in background mode, then run the import script through
Unreal's Python commandlet using the pattern in [ASSETS.md](ASSETS.md). The gate
is assembled at runtime and does not replace the saved pointer scene.
