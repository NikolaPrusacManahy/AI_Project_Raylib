# Raylib Basics in C

A tutorial covering the core raylib functions used for 2D graphics programming, with short standalone code examples for each topic.

## Table of Contents

1. [Creating a Window and the Game Loop](#1-creating-a-window-and-the-game-loop)
2. [Delta Time](#2-delta-time)
3. [Reading Keyboard Input](#3-reading-keyboard-input)
4. [Drawing Shapes](#4-drawing-shapes)
5. [Colors](#5-colors)
6. [Loading and Drawing Textures](#6-loading-and-drawing-textures)
7. [DrawTexturePro — Cropping, Scaling, and Flipping](#7-drawtexturepro--cropping-scaling-and-flipping)
8. [Spritesheet Animation](#8-spritesheet-animation)
9. [Switching Between Multiple Animations](#9-switching-between-multiple-animations)
10. [Bounds Checking (Simple Collision)](#10-bounds-checking-simple-collision)
11. [Simple Timers](#11-simple-timers)
12. [Useful Debug Helpers](#12-useful-debug-helpers)
13. [Quick Reference](#13-quick-reference)

---

## 1. Creating a Window and the Game Loop

Every raylib program opens a window, then repeatedly updates and draws inside a loop until the window is closed. The minimal complete program looks like this:

```c
#include "raylib.h"

int main(void)
{
    InitWindow(800, 450, "My First Raylib Window");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Hello, raylib!", 190, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

`InitWindow(width, height, title)` opens the OS window and prepares the graphics context — it must be the first raylib call. `WindowShouldClose()` becomes true once the user clicks the close button or presses ESC (the default close key), which ends the `while` loop. `SetTargetFPS(n)` caps how many frames run per second; raylib handles the timing internally.

---

## 2. Delta Time

`GetFrameTime()` returns how many seconds the previous frame took. Multiplying a speed value by this (`dt`) keeps motion consistent regardless of frame rate:

```c
// From main.c — frame-rate independent movement
float dt = GetFrameTime();
float playerSpeed = 250.0f; // pixels per second

if (IsKeyDown(KEY_RIGHT))
{
    playerPos.x += playerSpeed * dt;
}
```

Without multiplying by `dt`, an object would move a fixed number of pixels every frame, so it would appear to move faster on a machine running at 144 FPS than one running at 30 FPS. Using `dt` makes `playerSpeed` a true "pixels per second" value no matter the frame rate.

---

## 3. Reading Keyboard Input

Raylib provides three keyboard query functions, each suited to a different kind of input:

```c
// True every single frame the key is held down.
// Good for continuous actions like movement.
if (IsKeyDown(KEY_RIGHT)) { playerPos.x += speed * dt; }

// True only on the one frame the key first went down.
// Good for one-shot actions like jumping or attacking.
if (IsKeyPressed(KEY_SPACE)) { Jump(); }

// True only on the one frame the key was released.
if (IsKeyReleased(KEY_SPACE)) { StopCharging(); }
```

From `main.c`, the debug keys for triggering animations use `IsKeyPressed` so that holding the key down does not repeatedly restart the animation:

```c
// From main.c — debug animation triggers
if (IsKeyPressed(KEY_E)) { state = STATE_ATTACK; currentFrame = 0; frameTimer = 0.0f; }
if (IsKeyPressed(KEY_Y)) { state = STATE_HURT;   currentFrame = 0; frameTimer = 0.0f; }
if (IsKeyPressed(KEY_T)) { state = STATE_DEATH;  currentFrame = 0; frameTimer = 0.0f; }
```

Mouse and gamepad input follow the same three-function pattern: `IsMouseButtonDown/Pressed/Released` and `IsGamepadButtonDown/Pressed/Released`.

```c
if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
{
    Vector2 mousePos = GetMousePosition();
    TraceLog(LOG_INFO, "Clicked at %.0f, %.0f", mousePos.x, mousePos.y);
}
```

---

## 4. Drawing Shapes

Every drawing call must happen between `BeginDrawing()` and `EndDrawing()`. Nothing appears on screen until `EndDrawing()` runs. Raylib includes ready-made functions for basic shapes, useful for prototyping before real art is ready:

```c
BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawRectangle(100, 100, 200, 80, RED);
    DrawCircle(500, 140, 40, BLUE);
    DrawLine(50, 300, 750, 300, DARKGRAY);
    DrawText("Score: 0", 20, 20, 24, BLACK);
EndDrawing();
```

`ClearBackground(color)` wipes the previous frame's pixels; skipping it causes visual smearing. Draw order is also layering order — whatever is drawn first appears underneath whatever is drawn after it. `main.c` relies on this to draw the background image first, then the player sprite on top of it:

```c
// From main.c — draw order controls layering
BeginDrawing();
    ClearBackground(RAYWHITE);

    // Background drawn first -> renders behind everything else
    DrawTexturePro(background, bgSource, bgDest, (Vector2){ 0, 0 }, 0.0f, WHITE);

    // Player drawn after -> renders on top of the background
    DrawTexturePro(activeTex, sourceRec, destRec, origin, 0.0f, WHITE);
EndDrawing();
```

---

## 5. Colors

Colors are plain `Color` structs of four bytes (red, green, blue, alpha). Raylib predefines the common ones so custom colors are rarely needed:

```c
Color myPurple = { 130, 60, 200, 255 }; // r, g, b, alpha (0-255)

DrawRectangle(10, 10, 100, 40, RED);
DrawRectangle(10, 60, 100, 40, RAYWHITE);
DrawRectangle(10, 110, 100, 40, myPurple);

// Fade() returns a copy of a color with modified transparency (0.0 - 1.0)
DrawRectangle(10, 160, 100, 40, Fade(BLACK, 0.5f));
```

---

## 6. Loading and Drawing Textures

`LoadTexture(path)` reads an image file from disk and uploads it to the GPU one time, returning a `Texture2D` handle that can then be drawn every frame cheaply. It should always be called once, outside the game loop:

```c
Texture2D playerTexture = LoadTexture("resources/player_walk.png");

while (!WindowShouldClose())
{
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTexture(playerTexture, 100, 100, WHITE); // whole image, native size
    EndDrawing();
}

UnloadTexture(playerTexture); // free GPU memory once done
```

The file path is relative to the program's working directory at runtime (usually the project folder in Visual Studio), not to the `.c` source file itself. Every `LoadTexture` needs a matching `UnloadTexture` once the texture is no longer needed — `main.c` does this once per loaded texture, right before `CloseWindow()`:

```c
// From main.c — cleanup before closing
UnloadTexture(background);
UnloadTexture(texWalk);
UnloadTexture(texIdle);
UnloadTexture(texAttack);
UnloadTexture(texHurt);
UnloadTexture(texDeath);
CloseWindow();
```

---

## 7. DrawTexturePro — Cropping, Scaling, and Flipping

`DrawTexture` only draws a whole image at its native size. `DrawTexturePro` is more flexible: it takes a source rectangle (which region of the loaded image to sample) and a destination rectangle (where and at what size to draw it), which allows cropping, scaling, and flipping:

```c
Rectangle sourceRec = { 0, 0, (float)texture.width, (float)texture.height };
Rectangle destRec   = { x, y, drawWidth, drawHeight };
Vector2 origin       = { 0.0f, 0.0f };

DrawTexturePro(texture, sourceRec, destRec, origin, 0.0f, WHITE);
```

`main.c` uses this to scale the background image (originally 3840x2160) down to fill a 1280x720 window without distortion, since both share the same 16:9 aspect ratio:

```c
// From main.c — scaling a background to fill the window
Rectangle bgSource = { 0, 0, (float)background.width, (float)background.height };
Rectangle bgDest   = { 0, 0, (float)screenWidth, (float)screenHeight };
DrawTexturePro(background, bgSource, bgDest, (Vector2){ 0, 0 }, 0.0f, WHITE);
```

A negative width in the source rectangle flips the drawn image horizontally, which avoids needing a separate mirrored sprite for facing left vs. right:

```c
// From main.c — horizontal flip trick
Rectangle sourceRec = {
    (float)(currentFrame * frameWidth),
    0.0f,
    facingLeft ? -(float)frameWidth : (float)frameWidth, // negative = flipped
    (float)frameHeight
};
```

---

## 8. Spritesheet Animation

A spritesheet packs several animation frames into one image, laid out in a strip. `player_walk.png`, for example, is 6 walking frames of 32x32 pixels each, side by side:

```
[frame0][frame1][frame2][frame3][frame4][frame5]
```

The frame width is found by dividing the whole image's width by the frame count. A timer then advances `currentFrame` at a fixed rate, and the source rectangle offsets into the sheet by `currentFrame * frameWidth` to select which frame to draw:

```c
// From main.c — spritesheet frame stepping
const int frameCount = 6;
const int frameWidth = playerTexture.width / frameCount; // 32

int currentFrame = 0;
float frameTimer = 0.0f;
const float frameSpeed = 0.1f; // seconds per frame = 10 FPS animation

// inside the game loop, every frame:
frameTimer += dt;
if (frameTimer >= frameSpeed)
{
    frameTimer = 0.0f;
    currentFrame = (currentFrame + 1) % frameCount; // loop back to 0
}

Rectangle sourceRec = { (float)(currentFrame * frameWidth), 0, (float)frameWidth, (float)frameHeight };
```

This same pattern — divide by frame count, advance a timer, offset the source rectangle — is the standard approach for any 2D spritesheet animation in raylib, regardless of frame count or sheet size.

---

## 9. Switching Between Multiple Animations

A character with several animations (idle, walk, attack, etc.) can be modeled with an enum and a switch statement that picks which loaded texture and frame count to use each frame:

```c
// From main.c — animation state enum
typedef enum {
    STATE_IDLE,
    STATE_WALK,
    STATE_ATTACK,
    STATE_HURT,
    STATE_DEATH
} PlayerState;
```

```c
// From main.c — picking the active texture per frame
Texture2D activeTex = texIdle;
int frameCount = 4;
switch (state)
{
    case STATE_WALK:   activeTex = texWalk;   frameCount = 6; break;
    case STATE_ATTACK: activeTex = texAttack; frameCount = 4; break;
    case STATE_HURT:   activeTex = texHurt;   frameCount = 4; break;
    case STATE_DEATH:  activeTex = texDeath;  frameCount = 8; break;
    case STATE_IDLE:
    default:           activeTex = texIdle;   frameCount = 4; break;
}
int frameWidth = activeTex.width / frameCount;
```

Because each animation can have a different frame count, `frameWidth` must be recalculated every frame from whichever texture is currently active, rather than assumed to be a fixed constant.

---

## 10. Bounds Checking (Simple Collision)

The simplest form of collision detection is clamping a position so it cannot go past a boundary. After applying movement, check whether the new position is invalid and correct it before drawing:

```c
if (playerPos.x < 0) playerPos.x = 0;
if (playerPos.x > screenWidth - drawWidth) playerPos.x = screenWidth - drawWidth;
if (playerPos.y < 0) playerPos.y = 0;
if (playerPos.y > screenHeight - drawHeight) playerPos.y = screenHeight - drawHeight;
```

For collision between two rectangular objects (e.g. the player and an obstacle), raylib provides `CheckCollisionRecs`, which returns true if two `Rectangle` structs overlap:

```c
Rectangle player  = { playerPos.x, playerPos.y, 96, 96 };
Rectangle wall    = { 400, 300, 64, 64 };

if (CheckCollisionRecs(player, wall))
{
    // handle the overlap, e.g. stop movement or take damage
}
```

---

## 11. Simple Timers

Raylib has no built-in timer object; a timer is just a float that accumulates `GetFrameTime()` each frame until it reaches a target duration. This pattern is used for both animation timing and gameplay timing:

```c
// From main.c — holding on the last death frame before resetting
float deathHoldTimer = 0.0f;
const float deathHoldTime = 1.0f; // seconds to wait

// inside the game loop:
deathHoldTimer += dt;
if (deathHoldTimer >= deathHoldTime)
{
    deathHoldTimer = 0.0f;
    // trigger whatever should happen once the wait is over
}
```

---

## 12. Useful Debug Helpers

A few small functions are handy while developing, and safe to leave in or strip out later:

```c
DrawFPS(10, 10); // shows current frame rate in the corner

TraceLog(LOG_INFO, "Player position: %.1f, %.1f", playerPos.x, playerPos.y);
```

---

## 13. Quick Reference

| Function | Purpose |
|---|---|
| `InitWindow` / `CloseWindow` | Open and close the game window |
| `WindowShouldClose()` | True once the user wants to quit |
| `SetTargetFPS(n)` | Caps the frame rate |
| `GetFrameTime()` | Seconds since the last frame (`dt`) |
| `IsKeyDown` / `IsKeyPressed` / `IsKeyReleased` | Query keyboard state |
| `IsMouseButtonDown` / `Pressed` / `Released` | Query mouse state |
| `BeginDrawing` / `EndDrawing` | Bracket all drawing for one frame |
| `ClearBackground(color)` | Wipe the screen each frame |
| `DrawRectangle` / `DrawCircle` / `DrawLine` | Draw basic shapes |
| `DrawText` | Draw text on screen |
| `LoadTexture` / `UnloadTexture` | Load and free an image on the GPU |
| `DrawTexture` | Draw a whole image at native size |
| `DrawTexturePro` | Draw a cropped, scaled, or flipped region of an image |
| `CheckCollisionRecs` | Test whether two rectangles overlap |
| `Fade(color, alpha)` | Return a color with modified transparency |
| `DrawFPS` | Debug overlay showing current FPS |
| `TraceLog` | Print a debug message to the console |

---

## Further Reading

The official [raylib cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html) lists every available function grouped by category, and is the fastest way to discover functions not covered here.