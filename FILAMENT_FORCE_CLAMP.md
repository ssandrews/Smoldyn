# Ideal filament force clamp on an existing surface panel

Implemented locally on 2026-10-26. This is opt-in. It reuses Smoldyn surfaces,
rectangular panels, filament confinement forces, and the original boxes.

## Enable and disable

Add this **inside the existing surface block, after the named panel**:

```text
start_surface chamber
panel rect +2 -0.5 -0.5 -2 1 1 floor
panel rect -2 -0.5 -0.5  2 1 1 ceiling
# shape panel total_load force_tolerance min_coordinate max_coordinate
force_clamp rect ceiling 0.1 1e-8 -1.99 2
end_surface
```

- `0.1`: total inward applied load on this panel, in model force units. It is
  **not pressure**, force per filament, or pN unless the model uses pN. In the
  current standard model convention, one model force unit is 4.114 pN; this
  illustrative load is 0.4114 pN total. It is not a calibrated assay load.
- `1e-8`: absolute tolerance on the total reaction force, in the same force units.
  This is independent of `implicit_tolerance`, which checks filament displacement.
- `-1.99 2`: lower and upper allowed panel coordinates along its normal axis,
  in model length units. They must contain the current panel coordinate.
- The panel front must point into the filament-containing region. All filament
  types confined to this surface must use front-side confinement (the default),
  with `implicit_network` or fixed `dynamics none`. At least one filament type
  must select `implicit_network` globally. Keep `confine_surface chamber 400` in
  the filament type. The original wall stiffness and radius still apply.
- Disable with `force_clamp rect ceiling off`. The panel stays at its last
  accepted position and becomes an ordinary fixed panel.
- Remove/disable the old position-ramp command for this panel. Prescribed motion
  and an active force clamp are conflicting controls. The base branching membrane
  and other chamber panels remain fixed.

All rectangular normal axes in 2D and 3D are supported. Force refers to positive
outward reaction; the external load acts inward, opposite that reaction.
Multiple panels can be clamped independently, including opposing panels.

Runtime activation, load changes and disabling use the existing command/parser:

```text
cmd i 2 2 0.1 set surface chamber force_clamp rect ceiling 0.1 1e-8 -1.99 2
cmd i 10 10 0.1 set surface chamber force_clamp rect ceiling off
```

These are alternative examples, not instructions to apply both automatically.
A zero load is supported; its position is not unique once contact force is zero.

## Finite approach followed by constant load

Append `approach SPEED` to avoid jumping from the initial plate position to the
network on the first mechanical step:

```text
panel rect -2 -0.5 -0.5 -1.7 1 1 ceiling
force_clamp rect ceiling 0.1 1e-8 -1.99 2 approach 0.1
```

Here the fixed floor is at z=-2 um, so the initial plate clearance is 0.3 um.
The plate approaches inward at 0.1 um/s until the predicted next position has
filament reaction greater than the force tolerance. The controller then latches
into the existing ideal constant-load solve. Reaction below the target moves the
plate inward, reaction above it moves the plate outward, and a supported target
holds the plate at the force-balance position. After contact the required speed
comes from the coupled network mechanics, not the initial approach speed. There
is no added plate inertia, drag or proportional-gain controller. Force is not
maintained before contact; it cannot be applied to a network across an empty gap.

An unloaded approach stops at its inward travel limit and waits for contact.
With this option, travel limits remain physical stops after engagement: the
plate holds at a stop when the target is unreachable, reports the actual
filament reaction, and resumes force-balanced motion when support permits.
Force error can therefore be nonzero at a stop. Omitting `approach` retains
the original strict infeasibility error. Numerical failures still roll back the
whole interval. A contact appearing during a mechanical step can engage
the load controller on the following substep. The initial approach switch can
also produce a small displacement to establish the prescribed load.

The contact probe reuses the confinement force reduction once per mechanical
substep, from its accepted starting geometry. While approaching, the proposed
plate position is frozen across Newton/GMRES trials: those trials do not perform
extra plate force scans or change phase. After engagement the existing force
balance is required for each nonlinear residual; skipping it would make the
filament solve inconsistent. The cached analytical operator treats an approaching
panel or active travel stop as a fixed wall, and a freely moving engaged clamp
with its original height derivative.
No new filament collision searches, spatial grids or worker pools are added.
Small normal translations also skip panel-to-box membership updates when both
positions lie strictly within the same grid layer. Crossing or touching a grid
face retains the original membership checks and panel ordering.

The panel owns the approach speed and contact phase. Outer-interval rollback
restores both position and phase, including failures after earlier substeps.
`writesurfaces` includes `approach SPEED loaded` after engagement to preserve
that phase on serialization; `loaded` can also be specified explicitly to start
in force control. The nine existing `printForceClamps` columns are unchanged,
so recorded plate trajectories and the Simularium converter remain compatible.
Omitting the optional suffix selects the original immediate ideal clamp.

Project `Standard_experiment_force_clamp_approach.txt` uses this option with
separate output filenames, preserving the previous compression trajectory.

## How it couples to the implicit solve

For inward panel sign `s`, normal-axis coordinate `H`, node coordinate `z_i`,
radius `r_i` and existing node confinement stiffness `k_i`, define outward
coordinates `q=-s H` and `a_i=-s z_i+r_i`. Then

```
F(q) = sum_i k_i max(a_i - q, 0)
F(q) = target_load
```

The sum contains contributions from every filament type confined to the panel's
surface. Fixed filaments also supply support reactions. Other panels' forces,
internal filament forces, and filament-filament contact forces are not included.
The same contributions and signs are used by the original confinement helper.

For an active contact set, the force equation is linear:

```
q = (sum_active k_i a_i - target_load) / sum_active k_i
```

A bounded active-set Newton iteration solves this monotone equation for each
trial network geometry. A contact-seeking seed handles a piston initially above
the network. There is a separate force-error acceptance check and a 64-iteration
root-solve limit. This **algebraically eliminates** the piston coordinate instead
of adding another unknown to Newton/GMRES. GMRES finite differences therefore
include the piston's response to the perturbed network geometry. This is a
coupled ideal force clamp, not a lagged proportional controller or a speed ramp.

Only an optional structure on the original `panelstruct` is added. There are no
piston particles, wall particle meshes, additional spatial grids, or new numerical
libraries. Reaction reductions are serial and read-only; existing per-filament
force calculations remain parallel. Zero enabled clamps gives a fast inactive gate.

During trial evaluations only the panel's trial normal coordinate is changed;
`point`, `oldpoint`, molecule indexes, compartments and display geometry are not
modified. After **all mechanical substeps** succeed, the accepted position is
committed with `surftranslatepanel`. `boxesupdatepanel` refreshes only the original
boxes within the old/new swept rectangle bounds, retaining `panelinbox` and the
original panel-list order. Other payloads and the box resolution are unchanged.

Numerical failure or an infeasible travel bound restores all filament substeps
and the original panel position. Chemistry is retained, consistent with existing
implicit rollback. The solver does not silently abandon force control at a stop.
An empty network cannot support a nonzero load; activate the clamp after seeding
if the model initially has no filaments. Molecular pressure, feedback electronics,
fluid drag and deformable membrane tension/bending are not part of this force law.
The clamp does not add a load-dependent polymerization law: the existing elongation
rate and steric growth rules are unchanged. Matching an experimental force-velocity
curve requires validating those growth assumptions separately.

## Output

```text
output_files clamp_frames.txt clamp_load.txt
cmd i 0 100 0.1 printFilaments clamp_frames.txt
cmd i 0 100 0.1 printForceClamps clamp_load.txt
```

`printForceClamps` uses the ordinary command output/file system and flushes each
dump. It emits one line per enabled panel:

```text
CLAMP time surface:panel axis position target reaction error contact_stiffness
```

`error = reaction - target`; `contact_stiffness` is the sum of active per-node
wall stiffnesses. The observer recomputes the reaction from accepted geometry and
does not move the panel. The time-zero record precedes the first mechanical solve
and can therefore show the initially unloaded panel. Check force balance on
accepted records after loading, not the initial setup record.

`writesurfaces` / `savesim` preserve the current panel geometry and clamp settings;
this does not make FIL geometry a complete filament restart checkpoint. Existing
Simularium conversion can still show filaments, but a static `--plane` argument
does not animate this piston automatically: its positions are in `clamp_load.txt`.

## Examples and implementation map

- `examples/S13_filaments/force_clamp/constant_load.txt`: short, deterministic
  straight-filament demonstration.
- Project `Force_clamp_example.txt`: the same demo, convenient to run locally.
- Project `Standard_experiment_force_clamp.txt`: opt-in copy of the current
  standard experiment with illustrative load 0.1, separate output filenames and
  a load trace. The original standard input is unchanged.
- `smoldyn.h`: optional panel-owned state and enabled-count fast gate.
- `smolsurface.c`: parser/API, allocation/free, validation, serialization,
  force balance, and accepted surface translation.
- `smolfilament.c`: use trial panel coordinate in the existing confinement law.
- `smolfilamentimplicit.c`: solve the panel force balance between trial geometry
  installation and force evaluation, in both serial and parallel paths.
- `smolfilamentsteric.c`: commit only after all mechanical substeps succeed.
- `smolboxes.c`: targeted original-box panel membership updates. Also fixes a
  pre-existing ownership error: `expandboxpanels` must not free `box->segment`;
  that allocation is released by `boxfree`.
- `smolcmd.c`: `printForceClamps` observer.
- `tests/test_force_clamp.c`: native analytic, lifecycle and rollback checks.

Tests build with the existing `OPTION_FILAMENT_STERIC_TESTS=ON` gate. The feature
itself does not require that gate or OpenMP. See the project profiling report for
actual build, regression, runtime and performance measurements.
