#include "raylib.h"

int main(void)
{
    // ---- Window setup ----
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib 2D Template");
    SetTargetFPS(60);

    // ---- Load player spritesheet ----
    // player_walk.png is a walk-cycle sheet: 6 frames, 32x32 each, laid out horizontally
    Texture2D playerTexture = LoadTexture("resources/player_walk.png");

    const int frameCount = 6;
    const int frameWidth = playerTexture.width / frameCount; // 32
    const int frameHeight = playerTexture.height;             // 32
    const float scale = 3.0f; // draw the sprite bigger on screen

    int currentFrame = 0;
    float frameTimer = 0.0f;
    const float frameSpeed = 0.1f; // seconds per animation frame

    // ---- Player state ----
    Vector2 playerPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    float playerSpeed = 250.0f; // pixels per second
    bool facingLeft = false;
    bool isMoving = false;

    // Main game loop
    while (!WindowShouldClose()) // ESC or close button
    {
        // ---- Update ----
        float dt = GetFrameTime();
        isMoving = false;

        if (IsKeyDown(KEY_RIGHT)) { playerPos.x += playerSpeed * dt; facingLeft = false; isMoving = true; }
        if (IsKeyDown(KEY_LEFT))  { playerPos.x -= playerSpeed * dt; facingLeft = true;  isMoving = true; }
        if (IsKeyDown(KEY_DOWN))  { playerPos.y += playerSpeed * dt; isMoving = true; }
        if (IsKeyDown(KEY_UP))    { playerPos.y -= playerSpeed * dt; isMoving = true; }

        // Keep player on screen
        float drawWidth = frameWidth * scale;
        float drawHeight = frameHeight * scale;
        if (playerPos.x < 0) playerPos.x = 0;
        if (playerPos.y < 0) playerPos.y = 0;
        if (playerPos.x > screenWidth - drawWidth) playerPos.x = screenWidth - drawWidth;
        if (playerPos.y > screenHeight - drawHeight) playerPos.y = screenHeight - drawHeight;

        // Advance animation only while moving
        if (isMoving)
        {
            frameTimer += dt;
            if (frameTimer >= frameSpeed)
            {
                frameTimer = 0.0f;
                currentFrame = (currentFrame + 1) % frameCount;
            }
        }
        else
        {
            currentFrame = 0; // idle pose = first frame
            frameTimer = 0.0f;
        }

        // ---- Draw ----
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("Move with arrow keys", 10, 10, 20, DARKGRAY);

            // Source rectangle: pick the current frame out of the sheet.
            // Negative width flips the sprite horizontally when facing left.
            Rectangle sourceRec = {
                (float)(currentFrame * frameWidth),
                0.0f,
                facingLeft ? -(float)frameWidth : (float)frameWidth,
                (float)frameHeight
            };

            Rectangle destRec = { playerPos.x, playerPos.y, drawWidth, drawHeight };
            Vector2 origin = { 0.0f, 0.0f };

            DrawTexturePro(playerTexture, sourceRec, destRec, origin, 0.0f, WHITE);

            DrawFPS(10, screenHeight - 30);
        EndDrawing();
    }

    // ---- Cleanup ----
    UnloadTexture(playerTexture);
    CloseWindow();
    return 0;
}
