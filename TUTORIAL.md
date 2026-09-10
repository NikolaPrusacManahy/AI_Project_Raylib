# Raylib 2D Basics — A Walkthrough

This explains the core raylib concepts used in `main.c`, so you understand
*why* the code works, not just that it does. Every section references the
actual code in this project.

---

## 1. The Window and the Game Loop

Every raylib program follows the same three-part shape:

```c
InitWindow(screenWidth, screenHeight, "Title");  // 1. Open a window
SetTargetFPS(60);

while (!WindowShouldClose())                     // 2. Loop every frame
{
    // update game state
    // draw everything
}

CloseWindow();                                   // 3. Clean up
```

- `InitWindow` creates the OS window and sets up the graphics context. Must
  be called before any other raylib function.
- `WindowShouldClose()` returns `true` when the user hits the X button or
  presses ESC (by default). The `while` loop runs once per frame, forever,
  until this becomes true.
- `CloseWindow()` releases the window and graphics context. Always call it
  once you're done — but *not* before, since textures/sounds/etc. need a
  valid graphics context to load.

`SetTargetFPS(60)` caps the loop at 60 frames per second. Raylib handles the
waiting/timing for you — you don't need your own sleep/timer code.

---

## 2. Delta Time — Moving at the Same Speed on Any Machine

```c
float dt = GetFrameTime();
playerPos.x += playerSpeed * dt;
```

`GetFrameTime()` returns how many seconds the *last* frame took (e.g.
`0.016` at 60 FPS). Multiplying your speed by `dt` means movement is
measured in **units per second**, not **units per frame**.

Why this matters: if you just wrote `playerPos.x += playerSpeed` (no `dt`),
the player would move faster on a machine running 144 FPS than one running
30 FPS, because more frames = more additions per second. Multiplying by `dt`
cancels that out — `playerSpeed` becomes an actual real-world speed
(250 pixels per second, in this project).

---

## 3. Reading Input

```c
if (IsKeyDown(KEY_RIGHT)) { playerPos.x += playerSpeed * dt; }
```

- `IsKeyDown(key)` — true for every frame the key is held. Use this for
  continuous movement (what this project uses).
- `IsKeyPressed(key)` — true only on the single frame the key was first
  pressed. Use this for one-shot actions (jump, shoot, opening a menu).
- `IsKeyReleased(key)` — true on the frame a key is let go.

Raylib also has `IsMouseButtonDown/Pressed/Released` and gamepad
equivalents (`IsGamepadButtonDown`, etc.) that work the same way.

---

## 4. Drawing — Everything Happens Between BeginDrawing/EndDrawing

```c
BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Hello", 10, 10, 20, DARKGRAY);
EndDrawing();
```

- `BeginDrawing()` / `EndDrawing()` bracket every frame's drawing commands.
  Nothing you draw shows up until `EndDrawing()` is called.
- `ClearBackground(color)` wipes the screen at the start of every frame.
  Skip this and you'll get visual trails/smearing from the previous frame.
- **Draw order = layering order.** Whatever you draw first appears behind
  whatever you draw after it. This project draws the background image
  first, then the player on top of it, so the player renders above the
  scenery.

Colors are just `Color` structs — raylib predefines common ones (`RED`,
`WHITE`, `RAYWHITE`, `DARKGRAY`, etc.) so you rarely need to build your own.

---

## 5. Textures — Loading and Drawing Images

```c
Texture2D playerTexture = LoadTexture("resources/player_walk.png");
// ... use it every frame ...
UnloadTexture(playerTexture);
```

- `LoadTexture(path)` reads an image file from disk and uploads it to the
  GPU, returning a `Texture2D` handle. **Load once, outside the game loop**
  — never call this every frame, it's slow and leaks memory if you don't
  unload the old one first.
- `UnloadTexture(texture)` frees the GPU memory. Call it once, when you're
  completely done with the texture (this project does it right before
  `CloseWindow()`).
- The path is relative to your **working directory** at runtime, not the
  source file. In Visual Studio this is normally your project folder,
  which is why `resources/player_walk.png` works.

### Simple drawing: `DrawTexture`
```c
DrawTexture(texture, x, y, WHITE);
```
Draws the whole texture at `(x, y)` at its native size. Good enough for
static images like a single-frame background or icon.

### Flexible drawing: `DrawTexturePro`
This project uses `DrawTexturePro` instead, because it can crop a region
out of a larger image (for spritesheets) and scale/flip it:

```c
Rectangle sourceRec = { 0, 0, frameWidth, frameHeight };  // which part of the image
Rectangle destRec = { x, y, drawWidth, drawHeight };      // where + what size on screen
DrawTexturePro(texture, sourceRec, destRec, origin, rotation, WHITE);
```

- `sourceRec` — the rectangle *within the loaded image* to sample from.
  This is how spritesheet animation works (see next section).
- `destRec` — where to draw it on screen, and at what size. Since
  `destRec`'s width/height don't have to match `sourceRec`'s, this is also
  how you scale sprites up or down.
- A **negative width in `sourceRec`** flips the image horizontally — this
  project uses that trick to make the player face left without needing a
  separate mirrored sprite.

---

## 6. Spritesheet Animation

`player_walk.png` isn't one image — it's 6 walking frames laid out in a
horizontal strip, all the same size (32×32 pixels each):

```
[frame0][frame1][frame2][frame3][frame4][frame5]
```

To animate it, the code:

1. Figures out one frame's width by dividing the whole image's width by
   the frame count: `frameWidth = playerTexture.width / frameCount;`
2. Keeps a `currentFrame` counter (which frame is showing right now).
3. Every `frameSpeed` seconds (0.1s here — 10 frames/sec), advances
   `currentFrame` by one and wraps back to 0 after the last frame:
   ```c
   currentFrame = (currentFrame + 1) % frameCount;
   ```
4. When building `sourceRec` for `DrawTexturePro`, offsets the x-position
   by `currentFrame * frameWidth` — so each frame of the animation crops a
   different slice out of the same loaded texture.

This is the standard pattern for *any* spritesheet animation in 2D games —
only the frame count, size, and timing change.

---

## 7. Keeping Things on Screen (Bounds Clamping)

```c
if (playerPos.x < 0) playerPos.x = 0;
if (playerPos.x > screenWidth - drawWidth) playerPos.x = screenWidth - drawWidth;
```

After moving the player, the code checks if it went past a boundary and
snaps it back if so. This is the simplest possible form of collision —
"don't let X go below/above this line." The same pattern is used to stop
the player walking above the tree line into the background art:

```c
if (playerPos.y < groundLineY) playerPos.y = groundLineY;
```

More complex games extend this idea into full collision detection (checking
against walls, other objects, etc.) but it's the same core concept: detect
an invalid position, correct it before drawing.

---

## 8. Quick Reference

| Function | Purpose |
|---|---|
| `InitWindow` / `CloseWindow` | Open/close the game window |
| `WindowShouldClose()` | True when the user wants to quit |
| `SetTargetFPS(n)` | Cap the frame rate |
| `GetFrameTime()` | Seconds since the last frame (for `dt`) |
| `IsKeyDown` / `IsKeyPressed` | Read keyboard input |
| `BeginDrawing` / `EndDrawing` | Bracket all drawing for a frame |
| `ClearBackground(color)` | Wipe the screen each frame |
| `DrawText(text, x, y, size, color)` | Draw text |
| `LoadTexture(path)` / `UnloadTexture` | Load/free an image |
| `DrawTexture` | Draw a whole image at native size |
| `DrawTexturePro` | Draw a cropped/scaled/flipped region of an image |
| `DrawFPS(x, y)` | Debug overlay showing current FPS |

---

## Where to Go Next

- Try changing `playerSpeed`, `frameSpeed`, or `scale` in `main.c` and
  see how each affects the feel of the game.
- Add a second animation (idle vs. walking) by loading a second
  spritesheet and switching which `Texture2D` you draw from based on
  `isMoving`.
- Look up `CheckCollisionRecs` in the raylib cheatsheet
  (https://www.raylib.com/cheatsheet/cheatsheet.html) for proper
  rectangle-based collision between the player and other objects.
