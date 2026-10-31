# Geode Mods Active v0.1.1

Target: Geometry Dash 2.2081 / Geode 5.11.2.

## Controls
- **F6**: cycle input-buffer stages: 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF.
- **F7**: hide/show diagnostic console.
- **F8**: independently toggle Ship Predictive ON/OFF. This does not change the normal stage selection; when stages are OFF, Ship Predictive can still operate on ship input only.
- **F9**: master raw-input switch; disables every stage and Ship Predictive, clears pending buffered edges, and synchronizes held state.

## Input reliability notes
- Repeated DOWN/DOWN or UP/UP notifications are filtered while buffering is active; only physical state transitions are queued.
- Pending press/release edges are flushed in order when a new physical edge arrives, rather than waiting for a later update tick.
- Queued edges are dispatched only if they differ from the state currently held by the game.
- Death/restart clears pending edges and requires a genuine release if the button was held at death.

No auto-steering, auto-hold, gravity/speed/hitbox changes, or edits to the real player's physics. The mod only decides whether to delay a real input edge.

## Testing
Source-level changes only; this environment has not compiled the Geode mod or verified behavior in Geometry Dash. Test in Practice Mode first, especially F8/F9 transitions and dual mode. Absolute elimination of missed clicks cannot be guaranteed without reproducing the issue and testing against the exact game/input setup.

## Build
The included GitHub Actions workflow builds a Windows `.geode` artifact using the configured Geode SDK setup.
