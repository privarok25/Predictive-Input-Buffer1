# Predictive Input Buffer Prototype v0.4 — input queue + Raw Input mode

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## What changed from v0.2 / v0.3
- Added an experimental Ship Precision helper, toggled with **F8** (default OFF).
- With Ship Precision OFF, ship input follows the original v0.2 raw-input path.
- With Ship Precision ON, the helper may delay a user's real press/release within the existing F6 buffer limit when the built-in trajectory simulation predicts a better survival result.
- No automatic steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. The helper only buffers input; prediction runs on separate simulated PlayerObjects using the game's update/collision methods.
- F6 presets and F7 console visibility remain unchanged.

## Controls
- **F6**: cycle 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: toggle Ship Precision ON/OFF.
- **F9**: toggle Raw Input / Noclip Compatibility ON/OFF. ON bypasses prediction and sends real input edges directly through.

## Important testing note
This is an experimental source change and has **not been compiled or tested in-game in this environment**. Prediction based on a simulated PlayerObject may not perfectly reproduce every live-game state. Test first in practice mode with F8 OFF/ON and use F6 Stage 1 for the smallest intervention. Existing v0.2 behavior outside ship mode is intended to remain unchanged.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the project's configured Geode SDK setup.


## v0.4 fixes (experimental)

- **F9 — Raw Input / Noclip Compatibility:** bypasses trajectory prediction and input buffering, passing button edges directly through. Enable this for noclip runs because the mod cannot reliably detect every third-party noclip implementation automatically.
- **Queued input edges:** press/release/press transitions that arrive while a buffered input is pending are stored in order instead of being overwritten by a single queued-opposite flag. This targets missed fast Wave taps/releases.
- Original player physics is not modified. The change only affects when input edges are passed to the game.

This source has not been compiled or tested in-game in this environment. Test first in Practice Mode.
