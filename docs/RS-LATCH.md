# Facing-rod RS latch

Two copies of the [AND linkage](AND-GATE.md) face each other, with the second
rotated 180 degrees and displaced sideways by half the input spacing. Each
output axis therefore lines up with the other gate's feedback input. Because
the gates face opposite directions, an output displacement of 0 corresponds
to a receiving input displacement of 1, and vice versa.

The assembly uses the same meshes and pivot-to-pivot dimensions as the AND
demo, uniformly scaled by 0.64 to fit the table. Input spacing is 23.04 cm,
lateral gate offset is 11.52 cm, and the stroke is 21.888 cm. Both feedback
paths remain collinear, with no crossed rods. Cyan and orange handles are the
external Set and Reset controls; green ends indicate the feedback signals.

## Logic and use

| Set S' | Reset R' | Result |
| --- | --- | --- |
| 1 | 1 | Hold the previous state |
| 0 | 1 | Set Q=1, Q'=0 |
| 1 | 0 | Reset Q=0, Q'=1 |
| 0 | 0 | Invalid: Q=Q'=1 |

Use **Set pulse** or **Reset pulse** for an automatic 1.6-second assertion and
release. **Hold** releases both controls immediately. The input buttons, physical
handles and **Z / X** toggle the raw control rods, allowing the invalid case to
be explored. **Space** cycles Set → Hold → Reset → Hold; **R** pulses Reset.
The initial demonstration configuration is the valid hold state Q=0.

These are the active-low NAND-latch conventions described in
[TI's SR-latch explanation](https://www.ti.com/document-viewer/lit/html/SSZTCV9/GUID-2505F085-178B-4C5F-A9D1-9021A6A6C184).
The physical gates still calculate AND displacement; the reversed receiving
coordinate provides the inversion.

## Continuous feedback model

Let `F(a,b)` be the exact AND-linkage solver, `u,v` the two physical output
travels, and `b1,b2` the feedback input travels:

```text
u = F(S', b1)                 v = F(R', b2)
b1 approaches 1-v            b2 approaches 1-u
Q = 1-u                      Q' = 1-v
```

At settled Boolean endpoints this is `Q = NAND(S', Q')` and
`Q' = NAND(R', Q)`. No stored Boolean selects the gate geometry or forces a
remembered output. Both output positions come from fixed-length circle
intersections; the previous continuous positions carry the state.

The feedback connections are deliberately **compliant**, rendered as axial
springs around sliding cores. They accommodate differences between an output
and its receiving input during transitions. The simulation uses symmetric,
overdamped first-order followers, substepped at at most 1/240 second, rather
than imposing incompatible instantaneous positions on a closed rigid linkage.
This is a kinematic approximation, not a spring-force/rigid-body simulation or
proof that a fabricated mechanism will latch under load. At equilibrium the
connections converge to the inverted displacement relationship.

Both controls low gives the forbidden high/high logical outputs. A symmetrical
simultaneous release can sit near the unstable intermediate equilibrium
Q=Q'=0.47216. The UI uses `?` outside the end zones and reports **UNRESOLVED**;
an explicit Set or Reset pulse recovers. Physical asymmetry/noise could resolve
such a condition differently in a real mechanism.

## Validation and assets

`MaterialHall.Mechanism.RSLatch` tests both retained states, pulses, reversals,
invalid/recovery behavior and gate-exchange symmetry at 30, 60 and 144 fps.
`Tools/build-windows.ps1 -Action LatchTest` checks ten stages in the packaged
desktop app and writes JSON plus rendered views in `Saved/Validation/Run-*`.
It checks both linkages' visible joint alignment and both spring connections
throughout movement. `-Action GateTest` checks the single-gate regression.
Quest tests are not part of this desktop iteration.

`Tools/build_latch_meshes.py` generates the original axial coil in Blender.
`Tools/import_and_assets.py` imports it alongside the AND meshes. The assembly
is created by `AHallLatch`, with two embedded `AHallAndGate` actors; continuous
feedback is isolated in `HallLatchDynamics.h`.
