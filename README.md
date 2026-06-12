# Alpha Ring — master-chief

A modding tool for Halo: The Master Chief Collection (Steam, `1.3528.0.0`).
Works on Windows, Linux, and Steam Deck (Proton). This branch combines
thejackbitt's profile system, kadrim's Linux/Proton fixes, and a new
controller-driven splitscreen lobby.

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

1. Download `WTSAPI32.dll` from [Releases](../../releases) (`mc-*` tags for this branch) and drop it into
   `Halo The Master Chief Collection/MCC/Binaries/Win64/`
2. Launch MCC **with anti-cheat disabled**

**Linux / Steam Deck** — Steam launch options:

```
WINEDLLOVERRIDES="WTSAPI32=n,b" %command%
```

Optional: `ALPHARING_LOG=1` writes `AlphaRing.log` next to the DLL; `ALPHARING_WIREFRAME=1` enables the wireframe debug hook.

## Credits

Made by [WinterSquire](https://github.com/WinterSquire/AlphaRing); updated by xTrxplex and [wouter51](https://github.com/wouter51/AlphaRing); profile system, bind-by-press, and default mappings by [thejackbitt](https://github.com/thejackbitt/AlphaRing) (with kirklandsig and Priception); Linux/Proton fixes by [kadrim](https://github.com/kadrim/AlphaRing); lobby and this fork co-authored by devKreg.

Releases are built automatically by CI on every push.
