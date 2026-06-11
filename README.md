# Alpha Ring

A modding tool for Halo: The Master Chief Collection (Steam, `1.3528.0.0`).
Works on Windows, Linux, and Steam Deck (Proton).

## Features

- **Splitscreen** — up to 4 local players in every MCC game, each on their own controller, with per-player profiles and button mapping
- **Halo 3 toolbox** — dollycam (keyframed camera paths), object browser, player/physics editing, TAS input capture & playback
- In-game overlay menu, fully controller-navigable

## Install

1. Download `WTSAPI32.dll` from [Releases](../../releases/latest) and drop it into
   `Halo The Master Chief Collection/MCC/Binaries/Win64/`
2. Launch MCC **with anti-cheat disabled** (choose it in the MCC launcher)

**Linux / Steam Deck** — add to Steam launch options:

```
WINEDLLOVERRIDES="WTSAPI32=n,b" %command%
```

## Usage

- Toggle menu: `F4` or `Back + Start` on controller (menu starts hidden)
- Menu navigation with controller: right stick = cursor, `RB` = click
- Splitscreen: open menu → Splitscreen → Enable → set player count. Players default to Controller 1–4 in order. Once loaded into a mission, open the menu and hit **Load Profile** for each player.
- Mods (Workshop etc.) generally require **classic graphics**; Halo 2 campaign splitscreen is classic-only

Optional environment variables: `ALPHARING_LOG=1` writes `AlphaRing.log` next to the DLL; `ALPHARING_WIREFRAME=1` enables the wireframe debug hook.

## Credits

Made by [WinterSquire](https://github.com/WinterSquire/AlphaRing), updated by xTrxplex and [wouter51](https://github.com/wouter51/AlphaRing), Linux/Proton fixes by [kadrim](https://github.com/kadrim/AlphaRing), this fork co-authored by devKreg.

Releases are built automatically by CI on every push.
