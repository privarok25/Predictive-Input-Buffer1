# Geode Mods Active v0.1.1

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Controls
- **F6**: cycle input-buffer stages: 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: toggle Ship Predictive ON/OFF. When enabled, ship uses the same currently selected F6 stage as other modes: Stage 1 = 1 tick, Stage 2 = 2, Stage 3 = 4, Stage 4 = 8, Stage 5 = 16. Ship keeps a shorter 40-tick prediction horizon and only buffers an edge when a near-term collision is predicted and the delayed trajectory survives the horizon.
- **F9**: master raw-input switch; disables every stage and Ship Predictive, clears pending buffered edges, and synchronizes held state. F6 can cycle back to Stage 1; F8 can then re-enable ship prediction using that stage.

## Input reliability notes
- Repeated DOWN/DOWN or UP/UP notifications are filtered while buffering is active; only physical state transitions are queued.
- Pending press/release edges are flushed in order when a new physical edge arrives, rather than waiting for a later update tick.
- Queued edges are dispatched only if they differ from the state currently held by the game.
- Death/restart clears pending edges and requires a genuine release if the button was held at death.

No auto-steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. The mod only decides whether to delay a real input edge. Ship Predictive is not a separate strength setting: its maximum delay always follows the active F6 stage.

## Testing
Source-level changes only; this environment has not compiled the Geode mod or verified behavior in Geometry Dash. Test in Practice Mode first, especially F8/F9 transitions and dual mode. Absolute elimination of missed clicks cannot be guaranteed without reproducing the issue and testing against the exact game/input setup.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the configured Geode SDK setup.
