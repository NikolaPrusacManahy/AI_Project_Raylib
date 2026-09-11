#include "raylib.h"

// Player animation states
typedef enum {
    STATE_IDLE,
    STATE_WALK,
    STATE_ATTACK,
    STATE_HURT,
    STATE_DEATH
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

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        isMoving = false;

        // busy = a one-shot animation (attack/hurt/death) is currently playing
        bool busy = (state == STATE_ATTACK || state == STATE_HURT || state == STATE_DEATH);

        // ---- Debug keys: force a one-shot animation ----
        // Only allowed when not already busy, so spamming the key can't
        // interrupt/restart an animation that's still mid-playback.
        if (!busy)
        {
            if (IsKeyPressed(KEY_E)) { state = STATE_ATTACK; currentFrame = 0; frameTimer = 0.0f; busy = true; }
            else if (IsKeyPressed(KEY_Y)) { state = STATE_HURT;   currentFrame = 0; frameTimer = 0.0f; busy = true; }
            else if (IsKeyPressed(KEY_T)) { state = STATE_DEATH;  currentFrame = 0; frameTimer = 0.0f; deathHoldTimer = 0.0f; busy = true; }
        }

        // Movement only applies if not busy playing a debug animation
        if (!busy)
        {
            if (IsKeyDown(KEY_RIGHT)) { playerPos.x += playerSpeed * dt; facingLeft = false; isMoving = true; }
            if (IsKeyDown(KEY_LEFT)) { playerPos.x -= playerSpeed * dt; facingLeft = true;  isMoving = true; }
            if (IsKeyDown(KEY_DOWN)) { playerPos.y += playerSpeed * dt; isMoving = true; }
            if (IsKeyDown(KEY_UP)) { playerPos.y -= playerSpeed * dt; isMoving = true; }

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
                // One-shot animations (attack/hurt/death) stop instead of looping.
                // Idle/walk just loop back to frame 0.
                if (state == STATE_DEATH)
                {
                    // Stay on the last death frame; the hold timer below
                    // (not this frame-advance block) is what eventually
                    // resets back to idle.
                    currentFrame = frameCount - 1;
                }
                else if (busy) // attack or hurt finished
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

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle bgSource = { 0, 0, (float)background.width, (float)background.height };
        Rectangle bgDest = { 0, 0, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(background, bgSource, bgDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawRectangle(5, 5, 435, 30, BLACK);
        DrawText("Arrows: move  |  E: attack  Y: hurt  T: death", 10, 10, 20, WHITE);

        Rectangle sourceRec = {
            (float)(currentFrame * frameWidth),
            0.0f,
            facingLeft ? -(float)frameWidth : (float)frameWidth,
            (float)frameHeight
        };
        Rectangle destRec = { playerPos.x, playerPos.y, drawWidth, drawHeight };
        Vector2 origin = { 0.0f, 0.0f };

        DrawTexturePro(activeTex, sourceRec, destRec, origin, 0.0f, WHITE);

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
    CloseWindow();
    return 0;
}