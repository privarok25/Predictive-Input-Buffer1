# Geode Mods Action

Optimized and cleaned-up Geode input helpers for Geometry Dash 2.2081.

## Features
- Predictive input buffer with selectable stages (1/1, 2/2, 4/4, 8/8, 16/8, OFF)
- Ship predictive mode
- Global raw-input switch
- Per-click time budget: if prediction is too slow, the click is sent instantly and unchanged
- Silent: no console window, no popups, no in-game indicators

## Hotkeys
- **F6**: cycle buffer stages
- **F8**: toggle Ship Predictive
- **F9**: master raw-input switch (disables everything)
- **Ctrl + Shift + F10**: emergency kill switch. Instantly neutralizes the mod and disables it in Geode without closing the game (re-enable it in Geode > Mods)

The mod never touches the real player's physics, speed, gravity or hitbox. It only decides whether to delay a real input edge.
