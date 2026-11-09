# Geode Mods Action

**Geode Mods Action** is an experimental input-buffering and trajectory-prediction utility for Geometry Dash 2.2081, built for the Geode 5.11.2 mod loader.

## Features

- Configurable input-buffer stages selected with **F6**: `1/1 → 2/2 → 4/4 → 8/8 → 16/8 → OFF`.
- Optional **Ship Predictive** mode, toggled with **F8**. When enabled, it follows the currently selected F6 stage.
- **F9** disables all buffering stages and Ship Predictive, clears pending buffered input edges, and synchronizes the held-input state.
- Quiet startup: no automatically opened console, startup banner, save notifications, prediction messages, or in-level indicators.
- Simulation PlayerObjects are created lazily when a prediction is first needed.

## Input and gameplay behavior

The mod is designed to work with real player input. It does not steer the player automatically, hold buttons on its own, change gravity or speed, modify hitboxes, or directly alter the real player's physics. Its input-buffering logic may delay a real input edge according to the selected stage.

The existing game collision and physics routines are retained by this version. The hidden simulation objects are used for prediction; they are not intended to modify the actual player.

## Controls

| Key | Action |
| --- | --- |
| **F6** | Cycle buffering stages, including OFF |
| **F8** | Toggle Ship Predictive |
| **F9** | Disable all buffering and Ship Predictive |

## Compatibility

- Geometry Dash **2.2081** (Windows)
- Geode **5.11.2**

## Limitations and testing

This is an experimental utility. Prediction remains synchronous on the game thread because it uses live game objects and the game's collision routines. This release does not dynamically uninstall Geode hooks when disabled; hooks remain installed but are gated so ordinary gameplay should pass through when prediction is inactive.

The source has not been independently compiled or tested in-game as part of this metadata update. Test in Practice Mode first, including Wave, Ship, Dual, death/restart, stage changes, and F8/F9 transitions. No guarantee is made that every level pattern or input device behaves identically.


### Resource usage

The simulation player for Player 2 is created only when a Dual-mode prediction actually needs it. Single-player levels therefore avoid allocating and retaining an unused hidden PlayerObject. Prediction horizons and collision checks are intentionally unchanged to preserve behavior.
