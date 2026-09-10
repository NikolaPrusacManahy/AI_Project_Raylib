# Raylib 2D Template (Visual Studio)

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

## Files
- `main.c` — the game code (game loop + a movable square)
- `raylib2d.sln` / `raylib2d.vcxproj` — Visual Studio solution/project

## Controls
- Arrow keys: move the red square
- ESC: quit

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
