# Geode Mods Active v0.2.0 — Conservative Prediction Engine

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Core safety rule
This build does not change Geometry Dash movement, gravity, speed, hitboxes, collision rules, or level geometry. It does not steer or hold the player automatically. It only evaluates possible timings for an actual physical input edge and may delay that edge within the selected stage.

## Collision-candidate handling change
The previous prediction hook filtered the game's collision candidate list using a hard-coded `GameObjectType` whitelist. That could omit an object type the engine had already selected for collision processing, making an overlapping/stacked layout look like it contained a safe gap. v0.2.0 removes that second whitelist and forwards the exact candidate list to the original Geometry Dash collision routine. The game remains authoritative for collision semantics and hitboxes.

This is a targeted correction, not a claim that every broadphase limitation is solved: the mod still relies on the candidate list supplied by the engine for the simulated player's position. If the game itself supplies an incomplete list in a particular simulation context, that requires a deeper engine/API-level investigation.

## Performance
The per-tick temporary-vector construction, capacity checks, object filtering, and copying are removed from the simulation collision hook. The original collision routine receives the existing candidate vector directly. Prediction horizons and stage limits are intentionally unchanged so this build does not silently change the assistance strength.

## Controls
- **F6**: cycle stages: 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: toggle Ship Predictive; it uses the current global stage.
- **F9**: disable all stages and Ship Predictive (raw input).

## Testing
Source-level change only. This archive has not been compiled against the Geode SDK or tested in Geometry Dash in this environment. Test first in Practice Mode, especially overlapping structures, slopes, animated hazards, dual, wave and ship. Compare CPU usage and verify that ordinary inputs remain unchanged on safe sections.
