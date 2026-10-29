# Predictive Input Buffer Prototype v0.9 — simplified hotkeys

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Changes
- Ship input remains raw/manual; the experimental ship buffer is disabled.
- **F8** immediately forces every stage OFF, disables ship predictive input, clears pending buffered inputs, and synchronizes any held state.
- Kept queued input edges and the other existing controls.
- No auto-steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. This only decides whether to delay a real input edge.

## Controls
- **F6**: cycle 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: force all stages and ship prediction OFF (raw input).

## Testing note
This source has not been compiled or tested in-game in this environment. Test in Practice Mode first.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the configured Geode SDK setup.


## v0.7 input-edge fix
- Duplicate physical DOWN/UP notifications are ignored while buffering is enabled, preventing duplicate clicks from entering prediction/queue logic.
- Queued edges are dispatched only when they differ from the current game-held state.
- Raw-input mode (F6 OFF) is unchanged.
