# Alpha Ring

A modding tool for Halo: The Master Chief Collection (Steam, `1.3528.0.0`).
Works on Windows, Linux, and Steam Deck (Proton).

## Splitscreen lobby

Press **Start+Back** (or `F4`) to open the lobby:

1. 3-second countdown, then **Player 1 presses any button** to claim their controller
2. **Load a saved profile**, create a **new player** (on-screen keyboard, `Y` rolls a random Halo-themed name, `LT` toggles caps), or **edit** an existing profile
3. Confirm armor colors (swatch grid) and team
4. Next controller presses any button to join — or Player 1 presses **Start** to begin with the players bound so far (max 4)

Profiles (name, colors, team) persist in `alpha_ring_roster.json` next to the DLL, and player names show up in game. Reopening the lobby with a saved setup offers a quick **splitscreen on / single player / new setup** menu instead of full reconfiguration.

> **3+ players need classic graphics** (TAB toggles in game). The remaster renderer only supports two views — the lobby reminds you when a third player joins.

## Other features

- Per-player button mapping with bind-by-press (F1 debug menu)
- Halo 3 toolbox: dollycam, object browser, player/physics editing, TAS capture/playback
- Splitscreen quick-toggle persists across mission loads and restarts

## Install

1. Download `WTSAPI32.dll` from [Releases](../../releases/latest) and drop it into
   `Halo The Master Chief Collection/MCC/Binaries/Win64/`
2. Launch MCC **with anti-cheat disabled**

**Linux / Steam Deck** — Steam launch options:

```
WINEDLLOVERRIDES="WTSAPI32=n,b" %command%
```

Optional: `ALPHARING_LOG=1` writes `AlphaRing.log` next to the DLL; `ALPHARING_WIREFRAME=1` enables the wireframe debug hook.

Releases are built automatically by CI on every push.

## Credits

This fork was built with [Claude](https://claude.com/claude-code) and stands on great work by others:

- [WinterSquire/AlphaRing](https://github.com/WinterSquire/AlphaRing) — the original Alpha Ring, updated by xTrxplex
- [wouter51/AlphaRing](https://github.com/wouter51/AlphaRing) — updates for MCC `1.3528.0.0`
- [thejackbitt/AlphaRing](https://github.com/thejackbitt/AlphaRing) — profile system, bind-by-press controller binding, fixed default mappings (with kirklandsig and Priception)
- [kadrim/AlphaRing](https://github.com/kadrim/AlphaRing) — Linux/Proton fixes (mutexes, logging)
- [Assembly](https://github.com/XboxChaos/Assembly) for tag group research; [Blender](https://github.com/blender/blender) for bezier curve calculation
