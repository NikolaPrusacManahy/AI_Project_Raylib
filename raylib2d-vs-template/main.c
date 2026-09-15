#include "raylib.h"
#include <math.h>

// Player animation states
typedef enum {
    STATE_IDLE,
    STATE_WALK,
    STATE_ATTACK,
    STATE_HURT,
    STATE_DEATH,
    STATE_THROW
} PlayerState;

int main(void)
{
    // ---- Window setup ----
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib 2D Template");
    SetTargetFPS(60);

    // ---- Load background ----
    Texture2D background = LoadTexture("resources/game_background_1.png");
    const float groundLineY = screenHeight * 0.20f;

    // ---- Load player spritesheets ----
    // All sheets are 32px tall, frames laid out horizontally.
    Texture2D texWalk = LoadTexture("resources/player_walk.png");   // 6 frames
    Texture2D texIdle = LoadTexture("resources/player_idle.png");   // 4 frames
    Texture2D texAttack = LoadTexture("resources/player_attack.png"); // 4 frames
    Texture2D texHurt = LoadTexture("resources/player_hurt.png");   // 4 frames
    Texture2D texDeath = LoadTexture("resources/player_death.png");  // 8 frames
    Texture2D texThrow = LoadTexture("resources/player_throw.png");  // 4 frames

    Texture2D texRock = LoadTexture("resources/rock.png"); // single 16x16 sprite, not a sheet

    const float scale = 3.0f;
    const int frameHeight = 32;

    int currentFrame = 0;
    float frameTimer = 0.0f;
    const float frameSpeed = 0.1f; // seconds per animation frame

    // ---- Player state ----
    Vector2 playerPos = { screenWidth / 2.0f, screenHeight - 150.0f };
    float playerSpeed = 250.0f;
    bool facingLeft = false;
    bool isMoving = false;
    PlayerState state = STATE_IDLE;

    float deathHoldTimer = 0.0f;      // counts up once death's last frame is reached
    const float deathHoldTime = 1.0f; // seconds to hold on the last death frame

    // ---- Rock throw state ----
    // Only one rock in flight at a time - a single struct-like set of
    // variables is enough (no array needed for this pass).
    bool rockActive = false;
    Vector2 rockPos = { 0 };
    Vector2 rockVel = { 0 };
    Vector2 rockStartPos = { 0 }; // used to measure travel distance
    const float rockSpeed = 600.0f;      // pixels per second
    const float rockMaxDistance = 900.0f; // rock disappears after traveling this far
    const float rockScale = 2.0f;

    Vector2 throwAimDir = { 1.0f, 0.0f }; // direction locked in at the moment of throwing
    bool isAiming = false; // true while right-click is held down

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        isMoving = false;

        // busy = a one-shot animation (attack/hurt/death/throw) is currently playing
        bool busy = (state == STATE_ATTACK || state == STATE_HURT || state == STATE_DEATH || state == STATE_THROW);

        // ---- Debug keys: force a one-shot animation ----
        // Only allowed when not already busy or aiming, so spamming the key
        // can't interrupt/restart an animation that's still mid-playback,
        // and can't fire an attack mid-throw-aim.
        if (!busy && !isAiming)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { state = STATE_ATTACK; currentFrame = 0; frameTimer = 0.0f; busy = true; }
            else if (IsKeyPressed(KEY_Y)) { state = STATE_HURT;   currentFrame = 0; frameTimer = 0.0f; busy = true; }
            else if (IsKeyPressed(KEY_T)) { state = STATE_DEATH;  currentFrame = 0; frameTimer = 0.0f; deathHoldTimer = 0.0f; busy = true; }
        }

        // ---- Rock throw: hold right-click to aim, release to throw ----
        // rockActive being true blocks starting a new throw, so the player
        // can't spam rocks - only one can be in flight at a time.
        if (!busy && !rockActive)
        {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            {
                // While held, continuously update the aim direction so the
                // preview line follows the cursor (doesn't lock in yet).
                Vector2 mousePos = GetMousePosition();
                Vector2 playerCenter = {
                    playerPos.x + (frameHeight * scale) / 2.0f,
                    playerPos.y + (frameHeight * scale) / 2.0f
                };
                Vector2 toMouse = { mousePos.x - playerCenter.x, mousePos.y - playerCenter.y };
                float len = sqrtf(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
                if (len > 0.0001f) { toMouse.x /= len; toMouse.y /= len; }

                throwAimDir = toMouse;
                facingLeft = (toMouse.x < 0); // sprite still only flips left/right
                isAiming = true;
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && isAiming)
            {
                // Release fires the actual throw, using whichever direction
                // was last aimed - so tapping and holding both work.
                isAiming = false;
                state = STATE_THROW;
                currentFrame = 0;
                frameTimer = 0.0f;

                Vector2 playerCenter = {
                    playerPos.x + (frameHeight * scale) / 2.0f,
                    playerPos.y + (frameHeight * scale) / 2.0f
                };
                rockActive = true;
                rockPos = playerCenter;
                rockStartPos = playerCenter;
                rockVel.x = throwAimDir.x * rockSpeed;
                rockVel.y = throwAimDir.y * rockSpeed;
            }
        }

        // Movement only applies if not busy playing a debug animation
        if (!busy)
        {
            if (IsKeyDown(KEY_D)) { playerPos.x += playerSpeed * dt; facingLeft = false; isMoving = true; }
            if (IsKeyDown(KEY_A)) { playerPos.x -= playerSpeed * dt; facingLeft = true;  isMoving = true; }
            if (IsKeyDown(KEY_S)) { playerPos.y += playerSpeed * dt; isMoving = true; }
            if (IsKeyDown(KEY_W)) { playerPos.y -= playerSpeed * dt; isMoving = true; }

            state = isMoving ? STATE_WALK : STATE_IDLE;
        }

        // Keep player within screen bounds, and no higher than the ground line
        float drawWidth = frameHeight * scale;  // sprites are square (32x32), so width == height
        float drawHeight = frameHeight * scale;
        if (playerPos.x < 0) playerPos.x = 0;
        if (playerPos.x > screenWidth - drawWidth) playerPos.x = screenWidth - drawWidth;
        if (playerPos.y < groundLineY) playerPos.y = groundLineY;
        if (playerPos.y > screenHeight - drawHeight) playerPos.y = screenHeight - drawHeight;

        // ---- Pick which texture and frame count to use this frame ----
        Texture2D activeTex = texIdle;
        int frameCount = 4;
        switch (state)
        {
        case STATE_WALK:   activeTex = texWalk;   frameCount = 6; break;
        case STATE_ATTACK: activeTex = texAttack; frameCount = 4; break;
        case STATE_HURT:   activeTex = texHurt;   frameCount = 4; break;
        case STATE_DEATH:  activeTex = texDeath;  frameCount = 8; break;
        case STATE_THROW:  activeTex = texThrow;  frameCount = 4; break;
        case STATE_IDLE:
        default:           activeTex = texIdle;   frameCount = 4; break;
        }
        int frameWidth = activeTex.width / frameCount;

        // ---- Advance animation ----
        frameTimer += dt;
        if (frameTimer >= frameSpeed)
        {
            frameTimer = 0.0f;
            currentFrame++;

            if (currentFrame >= frameCount)
            {
                // One-shot animations (attack/hurt/death/throw) stop instead of looping.
                // Idle/walk just loop back to frame 0.
                if (state == STATE_DEATH)
                {
                    // Stay on the last death frame; the hold timer below
                    // (not this frame-advance block) is what eventually
                    // resets back to idle.
                    currentFrame = frameCount - 1;
                }
                else if (busy) // attack, hurt, or throw finished
                {
                    // Reset currentFrame to 0 before switching state so the
                    // next texture (idle) is never sampled with a leftover
                    // frame index it doesn't have - that caused the
                    // invisible-sprite bug.
                    state = STATE_IDLE;
                    currentFrame = 0;
                }
                else
                {
                    currentFrame = 0;
                }
            }
        }

        // Death holds on its last frame for a bit, then resets to idle.
        if (state == STATE_DEATH && currentFrame == frameCount - 1)
        {
            deathHoldTimer += dt;
            if (deathHoldTimer >= deathHoldTime)
            {
                deathHoldTimer = 0.0f;
                state = STATE_IDLE;
                currentFrame = 0;
            }
        }

        // ---- Update rock projectile ----
        if (rockActive)
        {
            rockPos.x += rockVel.x * dt;
            rockPos.y += rockVel.y * dt;

            // Straight-line distance from where the rock spawned to where it
            // is now (Pythagorean theorem) - used to despawn it after
            // rockMaxDistance, regardless of which direction it was thrown.
            float traveled = sqrtf(
                (rockPos.x - rockStartPos.x) * (rockPos.x - rockStartPos.x) +
                (rockPos.y - rockStartPos.y) * (rockPos.y - rockStartPos.y)
            );

            // 32px margin so the rock fully leaves the visible screen
            // before disappearing, instead of vanishing right at the edge.
            bool offScreen = (rockPos.x < -32 || rockPos.x > screenWidth + 32 ||
                rockPos.y < -32 || rockPos.y > screenHeight + 32);

            if (offScreen || traveled >= rockMaxDistance)
            {
                rockActive = false;
            }
        }

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle bgSource = { 0, 0, (float)background.width, (float)background.height };
        Rectangle bgDest = { 0, 0, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(background, bgSource, bgDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawRectangle(5, 5, 620, 30, BLACK);
        DrawText("WASD: move | Left-click: attack | Y: hurt T: death | Hold right-click: aim, release: throw", 10, 10, 16, WHITE);

        Rectangle sourceRec = {
            (float)(currentFrame * frameWidth),
            0.0f,
            facingLeft ? -(float)frameWidth : (float)frameWidth,
            (float)frameHeight
        };
        Rectangle destRec = { playerPos.x, playerPos.y, drawWidth, drawHeight };
        Vector2 origin = { 0.0f, 0.0f };

        DrawTexturePro(activeTex, sourceRec, destRec, origin, 0.0f, WHITE);

        // Aim line: visible the whole time right-click is held down,
        // so the player can see where the rock will go before releasing.
        if (isAiming)
        {
            Vector2 playerCenter = { playerPos.x + drawWidth / 2.0f, playerPos.y + drawHeight / 2.0f };
            Vector2 lineEnd = {
                playerCenter.x + throwAimDir.x * 60.0f,
                playerCenter.y + throwAimDir.y * 60.0f
            };
            DrawLineEx(playerCenter, lineEnd, 2.0f, Fade(YELLOW, 0.8f));
        }

        // Rock projectile
        if (rockActive)
        {
            float rockDrawSize = texRock.width * rockScale; // 16 * 2 = 32
            Rectangle rockSource = { 0, 0, (float)texRock.width, (float)texRock.height };
            // rockPos tracks the CENTER of the rock, but DrawTexturePro
            // positions from the top-left corner, so shift back by half
            // the draw size to keep it centered on rockPos.
            Rectangle rockDest = {
                rockPos.x - rockDrawSize / 2.0f,
                rockPos.y - rockDrawSize / 2.0f,
                rockDrawSize, rockDrawSize
            };
            DrawTexturePro(texRock, rockSource, rockDest, (Vector2) { 0, 0 }, 0.0f, WHITE);
        }

        DrawFPS(10, screenHeight - 30);
        EndDrawing();
    }

    // ---- Cleanup ----
    UnloadTexture(background);
    UnloadTexture(texWalk);
    UnloadTexture(texIdle);
    UnloadTexture(texAttack);
    UnloadTexture(texHurt);
    UnloadTexture(texDeath);
    UnloadTexture(texThrow);
    UnloadTexture(texRock);
    CloseWindow();
    return 0;
}