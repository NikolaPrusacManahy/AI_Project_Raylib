# Raylib 2D Template (Visual Studio)

A small top-down action game built with raylib in C: a player with movement,
melee attack, and a rock-throw projectile, versus an enemy that chases and
attacks on a cooldown.

## Project structure

```
raylib2d/
├── include/          # Public headers - struct definitions, constants, function declarations
│   ├── player.h
│   └── enemy.h
├── src/              # Implementation files
│   ├── main.c        # Window setup, game loop, cross-entity collision
│   ├── player.c
│   └── enemy.c
├── resources/         # Sprites and backgrounds
├── raylib2d.sln
└── raylib2d.vcxproj
```

Each entity (`player`, `enemy`) owns its own struct, `Init/Update/Draw/Unload`
functions, and tunable constants (speed, HP, damage, etc.) in its header.
`main.c` only handles what doesn't belong to either one alone: loading the
background, running the game loop, and checking collisions between the
player's attacks and the enemy.

## Setup

1. Download raylib's Windows release (the precompiled version) from:
   https://github.com/raysan5/raylib/releases
   Grab the `raylib-x.x.x_win64_msvc16` zip.

2. Extract it to `C:\raylib\raylib` so you end up with:
   ```
   C:\raylib\raylib\src\raylib.h
   C:\raylib\raylib\src\raylib.lib
   ```
   (If you put it somewhere else, open `raylib2d.vcxproj` and change the
   `<RaylibPath>` value near the top.)

3. Open `raylib2d.sln` in Visual Studio.

4. Set the build config to **x64** (top toolbar), then press **F5** or
   **Ctrl+F5** to build and run.

## Controls

- **WASD** — move
- **Left-click** — melee attack
- **Hold right-click** — aim a rock throw (see the yellow aim line);
  **release** to throw
- **Y** — debug: play the hurt animation
- **T** — debug: play the death animation
- **ESC** — quit

## Gameplay

- Player: 100 HP. Melee deals 25 damage, thrown rocks deal 50.
- Enemy: 50 HP. Walks toward the player, attacks for 10 damage once in
  range, on a 1-second cooldown. Slightly slower than the player.
- On death, the enemy plays its death animation and respawns after a delay
  at one of four fixed points around the edges of the screen.

## Troubleshooting

- **MSB8020: build tools for v143/v142 cannot be found**
  This means the project's toolset doesn't match what's installed. Two fixes,
  pick one:
  - **Retarget (easiest)**: right-click the solution in Solution Explorer →
    "Retarget solution" → OK. VS will switch the project to whichever
    toolset you actually have installed.
  - **Or install the matching component**: open "Visual Studio Installer" →
    Modify → under "Desktop development with C++" make sure "MSVC v143 -
    VS 2022 C++ x64/x86 build tools" is checked → Modify to install it.

- **LNK1104 raylib.lib not found**: double-check the `RaylibPath` in the
  `.vcxproj` matches where you extracted raylib.
- **Missing DLL errors on run**: use the `_win64_msvc16` build (static lib),
  not the dynamic one, or copy `raylib.dll` next to your `.exe`.
- **Cannot open include file 'player.h'**: the project's Additional Include
  Directories must include the project's `include\` folder - this is
  already configured in `raylib2d.vcxproj`, so this only comes up if the
  project file itself wasn't picked up correctly.
