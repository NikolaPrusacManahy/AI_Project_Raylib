#include "raylib.h"
#include "player.h"
#include "enemy.h"
#include <math.h>

int main(void)
{
    // ---- Window setup ----
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib 2D Template");
    SetTargetFPS(60);

    // ---- Load backgrounds ----
    // Two maps: the starting area, and the one unlocked after enough kills.
    Texture2D background1 = LoadTexture("resources/game_background_1.png");
    Texture2D background2 = LoadTexture("resources/game_background_2.png");
    Texture2D currentBackground = background1;
    bool onSecondMap = false; // tracks which map is active - can't compare Texture2D structs directly
    const float groundLineY = screenHeight * 0.20f;

    // ---- Player ----
    Player player;
    InitPlayer(&player, (Vector2) { screenWidth / 2.0f, screenHeight - 150.0f });

    // ---- Enemies ----
    // All ENEMY_COUNT enemies share one set of loaded textures (they use
    // the same art) - only their individual state differs.
    EnemyTextures enemyTex;
    LoadEnemyTextures(&enemyTex);

    InitEnemySpawnPoints(screenWidth, screenHeight, groundLineY);
    Enemy enemies[ENEMY_COUNT];
    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        // Spread the initial spawns across the 4 fixed points so they
        // don't all start stacked on the same one.
        InitEnemy(&enemies[i], g_enemySpawnPoints[i % 4]);
    }

    // Tracks whether the current attack swing has already landed a hit,
    // so one swing can't damage the same enemy on every frame its active
    // window is open - only once per swing. One flag per enemy, since the
    // player's single swing could in principle overlap more than one.
    bool meleeHitLanded[ENEMY_COUNT] = { false };

    int killCount = 0;
    bool exitUnlocked = false; // true once killCount reaches KILLS_TO_UNLOCK_EXIT

    // F1 toggles drawing the hitboxes as outlines, for visually checking
    // collision sizing against the sprites instead of guessing at numbers.
    bool showHitboxes = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        UpdatePlayer(&player, dt, screenWidth, screenHeight, groundLineY, exitUnlocked);

        // Reset each enemy's "already hit" flag whenever the player isn't
        // mid-swing, so the next attack can land on each of them again.
        if (player.state != STATE_ATTACK)
        {
            for (int i = 0; i < ENEMY_COUNT; i++) meleeHitLanded[i] = false;
        }

        for (int i = 0; i < ENEMY_COUNT; i++)
        {
            Enemy* e = &enemies[i];
            UpdateEnemy(e, &player, dt, screenWidth, screenHeight, groundLineY, enemies, ENEMY_COUNT);

            // ---- Melee: only during the attack's active frames, only once per swing ----
            if (PlayerAttackIsActive(&player) && !meleeHitLanded[i] && e->state != ENEMY_DEAD)
            {
                if (CheckCollisionRecs(PlayerBounds(&player), EnemyBounds(e)))
                {
                    if (EnemyTakeDamage(e, PLAYER_ATTACK_DAMAGE)) killCount++;
                    meleeHitLanded[i] = true;
                }
            }

            // ---- Rock vs this enemy ----
            if (player.rockActive && e->state != ENEMY_DEAD)
            {
                float rockDrawSize = player.texRock.width * ROCK_SCALE;
                Rectangle rockBounds = {
                    player.rockPos.x - rockDrawSize / 2.0f,
                    player.rockPos.y - rockDrawSize / 2.0f,
                    rockDrawSize, rockDrawSize
                };
                if (CheckCollisionRecs(rockBounds, EnemyBounds(e)))
                {
                    if (EnemyTakeDamage(e, ROCK_DAMAGE)) killCount++;
                    player.rockActive = false; // rock is consumed on hit
                }
            }
        }

        if (killCount >= KILLS_TO_UNLOCK_EXIT) exitUnlocked = true;

        // ---- Map transition ----
        // Once unlocked, UpdatePlayer (above) stops clamping the right
        // edge, so the player can actually walk past screenWidth. Once
        // they do, swap to the second map and re-enter from the left.
        if (exitUnlocked && player.pos.x > screenWidth)
        {
            currentBackground = background2;
            onSecondMap = true;
            player.pos.x = 0.0f;
        }

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Rectangle bgSource = { 0, 0, (float)currentBackground.width, (float)currentBackground.height };
        Rectangle bgDest = { 0, 0, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(currentBackground, bgSource, bgDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawRectangle(5, 5, 620, 30, BLACK);
        DrawText("WASD: move | Left-click: attack | Hold right-click: aim, release: throw", 10, 10, 16, WHITE);

        DrawText(TextFormat("Kills: %d / %d", killCount, KILLS_TO_UNLOCK_EXIT), 10, screenHeight - 105, 20,
            exitUnlocked ? LIME : WHITE);

        for (int i = 0; i < ENEMY_COUNT; i++) DrawEnemy(&enemies[i], &enemyTex);
        DrawPlayer(&player);
        DrawPlayerHealthBar(&player);

        // Hovering arrow pointing right, telling the player to head out of
        // bounds, shown only once enough enemies have been killed.
        if (exitUnlocked && !onSecondMap)
        {
            float bob = sinf((float)GetTime() * 3.0f) * 8.0f; // gentle up/down float
            float arrowY = screenHeight / 2.0f + bob;
            float arrowX = screenWidth - 60.0f;

            Vector2 p1 = { arrowX - 20, arrowY - 25 };
            Vector2 p2 = { arrowX - 20, arrowY + 25 };
            Vector2 p3 = { arrowX + 20, arrowY };
            DrawTriangle(p1, p2, p3, GOLD);
            DrawTriangleLines(p1, p2, p3, BLACK);

            const char* msg = "Head right!";
            int msgWidth = MeasureText(msg, 20);
            DrawText(msg, (int)(arrowX - msgWidth / 2.0f), (int)(arrowY - 55), 20, GOLD);
        }

        if (showHitboxes)
        {
            DrawRectangleLinesEx(PlayerBounds(&player), 2.0f, LIME);
            for (int i = 0; i < ENEMY_COUNT; i++)
            {
                if (enemies[i].state != ENEMY_DEAD)
                {
                    DrawRectangleLinesEx(EnemyBounds(&enemies[i]), 2.0f, LIME);
                }
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
    UnloadTexture(background1);
    UnloadTexture(background2);
    UnloadPlayer(&player);
    UnloadEnemyTextures(&enemyTex);
    CloseWindow();
    return 0;
}