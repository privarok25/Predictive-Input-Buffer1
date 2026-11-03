# Geode Mods Action v0.1.2

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Controls
- **F6**: cycle input-buffer stages: 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F8**: toggle Ship Predictive ON/OFF. When enabled, ship uses the currently selected F6 stage.
- **F9**: master raw-input switch; disables every stage and Ship Predictive, clears pending buffered edges, and synchronizes held state. F6 can cycle back to Stage 1.

- **Ctrl+Shift+F10**: emergency kill switch (works anywhere, game stays open). Change `KILL_VK` in main.cpp to rebind.

## Quiet behavior
- No console window is created on mod load.
- No startup banner, save notifications, or per-prediction console messages are emitted.
- No in-game indicators are added.
- F7 no longer has a console action.

## Incremental safety / performance changes
- Hidden simulation PlayerObjects are created lazily, only when an actual prediction is requested, instead of during every PlayLayer initialization.
- Internal buffered dispatch has a final state-transition guard to avoid sending duplicate synthetic DOWN/DOWN or UP/UP edges.
- Existing queue ordering, prediction horizons, stage values, and original collision/physics routines are otherwise retained in this pass.

## Important limitations
- Geode Modify hooks are installed by the mod's hook system; this implementation does not dynamically uninstall them when prediction is disabled. The hooks are gated and should pass through ordinary game behavior when simulation is inactive. Full runtime hook uninstallation is not implemented.
- Accurate prediction currently depends on live game objects and original collision routines, so the simulation remains synchronous on the game thread. Moving it to a worker thread without a safe snapshot of all relevant game state would risk races and incorrect physics. This pass does not claim to solve that.
- The deeper CPU optimization, full stale-edge/physical-state audit, and broader collision-object equivalence testing are intentionally not included until measured and tested separately.

No auto-steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. The mod only decides whether to delay a real input edge.

## Testing
Source-level checks only. This environment has not compiled the Geode mod or verified behavior in Geometry Dash. Test in Practice Mode first, especially stage changes, F8/F9 transitions, death/restart, and dual mode. Absolute elimination of missed clicks cannot be guaranteed without testing against the exact game/input setup.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the configured Geode SDK setup.

