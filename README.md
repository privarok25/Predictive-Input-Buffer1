# Predictive Input Buffer Prototype v0.5 — conservative Ship Precision

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Changes
- Ship Precision remains opt-in via **F8** (default OFF).
- Ship prediction now uses a short 40-tick lookahead instead of the long general horizon.
- A ship input is buffered only when the immediate input predicts a very near collision (within 20 simulation ticks) and a small delay survives the entire short horizon. Long straight-fly sections therefore remain raw/manual input and are not buffered based on speculative distant predictions.
- Removed **F9** and its Raw Input / Noclip Compatibility mode.
- Kept queued input edges and the other existing controls.
- No auto-steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. This only decides whether to delay a real input edge.

## Controls
- **F6**: cycle 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: toggle Ship Precision ON/OFF.

## Testing note
This source has not been compiled or tested in-game in this environment. Test in Practice Mode first. Conservative ship timing reduces false positives but cannot guarantee perfect agreement between simulated and live game state.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the configured Geode SDK setup.
