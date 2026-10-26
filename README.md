# Predictive Input Buffer Prototype

Prototype project for Geometry Dash 2.2081 / Geode 5.11.2.

## Change in this prototype
If a run ends while the physical input is still held, the buffer discards pending actions and waits for a genuine release before accepting another press. This is intended to prevent a synthetic re-press/state resynchronization after death.

## Build
The GitHub Actions workflow builds a Windows `.geode` artifact. This source project has not been compiled or tested in-game in this environment; treat the resulting file as a test build.
