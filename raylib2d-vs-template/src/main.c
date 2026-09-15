#include "raylib.h"
#include "player.h"
#include "enemy.h"

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

    // ---- Player and enemy ----
    Player player;
    InitPlayer(&player, (Vector2) { screenWidth / 2.0f, screenHeight - 150.0f });

    InitEnemySpawnPoints(screenWidth, screenHeight, groundLineY);
    Enemy enemy;
    InitEnemy(&enemy, g_enemySpawnPoints[0]);

    // Tracks whether the current attack swing has already landed a hit,
    // so one swing can't damage the enemy on every frame its active
    // window is open - only once per swing.
    bool meleeHitLanded = false;

    // F1 toggles drawing the hitboxes as outlines, for visually checking
    // collision sizing against the sprites instead of guessing at numbers.
    bool showHitboxes = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        UpdatePlayer(&player, dt, screenWidth, screenHeight, groundLineY);
        UpdateEnemy(&enemy, &player, dt, screenWidth, screenHeight, groundLineY);

        // Reset the "already hit" flag whenever the player isn't mid-swing,
        // so the next attack can land again.
        if (player.state != STATE_ATTACK) meleeHitLanded = false;

        // ---- Melee: only during the attack's active frames, only once per swing ----
        if (PlayerAttackIsActive(&player) && !meleeHitLanded && enemy.state != ENEMY_DEAD)
        {
            if (CheckCollisionRecs(PlayerBounds(&player), EnemyBounds(&enemy)))
            {
                EnemyTakeDamage(&enemy, PLAYER_ATTACK_DAMAGE);
                meleeHitLanded = true;
            }
        }

        // ---- Rock vs enemy ----
        if (player.rockActive && enemy.state != ENEMY_DEAD)
        {
            float rockDrawSize = player.texRock.width * ROCK_SCALE;
            Rectangle rockBounds = {
                player.rockPos.x - rockDrawSize / 2.0f,
                player.rockPos.y - rockDrawSize / 2.0f,
                rockDrawSize, rockDrawSize
            };
            if (CheckCollisionRecs(rockBounds, EnemyBounds(&enemy)))
            {
                EnemyTakeDamage(&enemy, ROCK_DAMAGE);
                player.rockActive = false; // rock is consumed on hit
            }
        }

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle bgSource = { 0, 0, (float)background.width, (float)background.height };
        Rectangle bgDest = { 0, 0, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(background, bgSource, bgDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawRectangle(5, 5, 620, 30, BLACK);
        DrawText("WASD: move | Left-click: attack | Hold right-click: aim, release: throw", 10, 10, 16, WHITE);

        DrawEnemy(&enemy);
        DrawPlayer(&player);
        DrawPlayerHealthBar(&player);

        if (showHitboxes)
        {
            DrawRectangleLinesEx(PlayerBounds(&player), 2.0f, LIME);
            if (enemy.state != ENEMY_DEAD)
            {
                DrawRectangleLinesEx(EnemyBounds(&enemy), 2.0f, LIME);
            }
            DrawText("F1: hide hitboxes", 10, screenHeight - 55, 16, LIME);
        }
        else
        {
            DrawText("F1: show hitboxes", 10, screenHeight - 55, 16, GRAY);
        }

        DrawFPS(10, screenHeight - 30);
        EndDrawing();
    }

    // ---- Cleanup ----
    UnloadTexture(background);
    UnloadPlayer(&player);
    UnloadEnemy(&enemy);
    CloseWindow();
    return 0;
}