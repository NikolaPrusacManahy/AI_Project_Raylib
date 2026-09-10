#include "raylib.h"

int main(void)
{
    // Window setup
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Raylib 2D Template");
    SetTargetFPS(60);

    // Player
    Vector2 playerPos = { screenWidth / 2.0f, screenHeight / 2.0f };
    float playerSpeed = 200.0f; // pixels per second
    float playerSize = 40.0f;

    // Main game loop
    while (!WindowShouldClose()) // ESC or close button
    {
        // --- Update ---
        float dt = GetFrameTime();

        if (IsKeyDown(KEY_RIGHT)) playerPos.x += playerSpeed * dt;
        if (IsKeyDown(KEY_LEFT))  playerPos.x -= playerSpeed * dt;
        if (IsKeyDown(KEY_DOWN))  playerPos.y += playerSpeed * dt;
        if (IsKeyDown(KEY_UP))    playerPos.y -= playerSpeed * dt;

        // Keep player on screen
        if (playerPos.x < 0) playerPos.x = 0;
        if (playerPos.y < 0) playerPos.y = 0;
        if (playerPos.x > screenWidth - playerSize) playerPos.x = screenWidth - playerSize;
        if (playerPos.y > screenHeight - playerSize) playerPos.y = screenHeight - playerSize;

        // --- Draw ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("Move with arrow keys", 10, 10, 20, DARKGRAY);
            DrawRectangleV(playerPos, (Vector2){ playerSize, playerSize }, RED);

            DrawFPS(10, screenHeight - 30);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
