#include "raylib.h"
#include "player.h"
#include "enemy.h"
#include <math.h>

// Resets every piece of run state back to a fresh game - player, all
// enemies, the kill counter, the map, and the death/timer flags. Takes
// pointers to everything main() owns so it can reset them in place
// instead of returning a big struct of "new" values.
static void RestartGame(Player* player, Enemy* enemies, EnemyTextures* enemyTex,
    bool* meleeHitLanded, int* killCount, bool* exitUnlocked,
    bool* enemiesCanRespawn, float* survivalTime, bool* gameOver,
    Texture2D* currentBackground, Texture2D background1, bool* onSecondMap,
    int screenWidth, int screenHeight, float groundLineY)
{
    // Unload the previous run's textures before InitPlayer loads a fresh
    // set - otherwise every restart leaks the old ones (they're never
    // freed, just replaced).
    UnloadPlayer(player);
    InitPlayer(player, (Vector2) { screenWidth - 200.0f, screenHeight - 150.0f });

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        InitEnemy(&enemies[i], g_enemySpawnPoints[i % 4]);
        meleeHitLanded[i] = false;
    }
    (void)enemyTex; // textures are loaded once and reused - nothing to reset here

    *killCount = 0;
    *exitUnlocked = false;
    *enemiesCanRespawn = true;
    *survivalTime = 0.0f;
    *gameOver = false;

    *currentBackground = background1;
    *onSecondMap = false;
}

int main(void)
{
    // ---- Window setup ----
    const int screenWidth = 1440;
    const int screenHeight = 880;
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
    // Spawns on the right side of the screen, away from the enemy spawn
    // points (which are spread around the edges) - starting dead-center
    // put the player too close to a spawning enemy immediately.
    Player player;
    InitPlayer(&player, (Vector2) { screenWidth - 200.0f, screenHeight - 150.0f });

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

    int killCount = 0; // total enemies killed so far, across all 3
    bool exitUnlocked = false; // true once killCount reaches KILLS_TO_UNLOCK_EXIT

    // Once the kill target is reached, dead enemies stop respawning -
    // enemiesCanRespawn is passed into UpdateEnemy for every enemy.
    // (On the second map there's no limit, so this stays true there too
    // once the player has crossed over - see the transition block below.)
    bool enemiesCanRespawn = true;

    // ---- Death / game-over state ----
    float survivalTime = 0.0f; // seconds survived, stops counting once dead
    bool gameOver = false;     // true once the death animation has finished playing

    // F1 toggles drawing the hitboxes as outlines, for visually checking
    // collision sizing against the sprites instead of guessing at numbers.
    bool showHitboxes = false;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        // Restart: only listened for while the death screen is showing.
        if (gameOver && IsKeyPressed(KEY_SPACE))
        {
            RestartGame(&player, enemies, &enemyTex, meleeHitLanded, &killCount,
                &exitUnlocked, &enemiesCanRespawn, &survivalTime, &gameOver,
                &currentBackground, background1, &onSecondMap,
                screenWidth, screenHeight, groundLineY);
        }

        if (!gameOver)
        {
            survivalTime += dt;

            UpdatePlayer(&player, dt, screenWidth, screenHeight, groundLineY, exitUnlocked);

            // Reset each enemy's "already hit" flag whenever the player
            // isn't mid-swing, so the next attack can land on each again.
            if (player.state != STATE_ATTACK)
            {
                for (int i = 0; i < ENEMY_COUNT; i++) meleeHitLanded[i] = false;
            }

            for (int i = 0; i < ENEMY_COUNT; i++)
            {
                Enemy* e = &enemies[i];
                UpdateEnemy(e, &player, dt, screenWidth, screenHeight, groundLineY,
                    enemies, ENEMY_COUNT, enemiesCanRespawn);

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

            // Stop new enemies from replacing dead ones once the first-map
            // kill target is reached. Once the player actually crosses
            // into map 2, respawning turns back on with no further limit
            // (see the transition block below).
            if (!onSecondMap && killCount >= KILLS_TO_UNLOCK_EXIT)
            {
                exitUnlocked = true;
                enemiesCanRespawn = false;
            }

            // ---- Map transition ----
            // Once unlocked, UpdatePlayer (above) stops clamping the right
            // edge, so the player can actually walk past screenWidth. Once
            // they do, swap to the second map and re-enter from the left.
            if (exitUnlocked && !onSecondMap && player.pos.x > screenWidth)
            {
                currentBackground = background2;
                onSecondMap = true;
                player.pos.x = 0.0f;
                enemiesCanRespawn = true; // no kill limit on the second map
            }

            // ---- Check for death ----
            // PlayerDeathFinished is true once the death animation has
            // fully played (not the instant hp hits 0), matching how the
            // other one-shot animations play out before anything reacts.
            if (PlayerDeathFinished(&player))
            {
                gameOver = true;
            }
        }

        // ---- Draw ----
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Source = the whole background image; dest = the full window, so
        // it stretches to exactly fill the screen regardless of native size.
        Rectangle bgSource = { 0, 0, (float)currentBackground.width, (float)currentBackground.height };
        Rectangle bgDest = { 0, 0, (float)screenWidth, (float)screenHeight };
        DrawTexturePro(currentBackground, bgSource, bgDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawRectangle(5, 5, 620, 30, BLACK);
        DrawText("WASD: move | Left-click: attack | Hold right-click: aim, release: throw", 10, 10, 16, WHITE);

        // Kill counter: shows "/ 7" and the target while on the first map,
        // just the running total once on the second map (no limit there).
        // Always plain white now, regardless of unlock state.
        if (onSecondMap)
        {
            DrawText(TextFormat("Kills: %d", killCount), 10, screenHeight - 105, 20, WHITE);
        }
        else
        {
            DrawText(TextFormat("Kills: %d / %d", killCount, KILLS_TO_UNLOCK_EXIT), 10, screenHeight - 105, 20, WHITE);
        }

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

            // The 3 corner points of a right-pointing triangle: two on the
            // left (top and bottom), one on the right (the tip).
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

        // ---- Death screen overlay ----
        // Drawn last so it sits on top of everything else; the game loop
        // above is frozen (see the `if (!gameOver)` guard) so this just
        // shows over whatever the last live frame looked like.
        if (gameOver)
        {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));

            const char* title = "YOU DIED";
            int titleSize = 60;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, (screenWidth - titleWidth) / 2, screenHeight / 2 - 100, titleSize, RED);

            const char* scoreText = TextFormat("Score: %d kills", killCount);
            int scoreSize = 30;
            int scoreWidth = MeasureText(scoreText, scoreSize);
            DrawText(scoreText, (screenWidth - scoreWidth) / 2, screenHeight / 2 - 10, scoreSize, WHITE);

            const char* timeText = TextFormat("Time lasted: %d seconds", (int)survivalTime);
            int timeWidth = MeasureText(timeText, scoreSize);
            DrawText(timeText, (screenWidth - timeWidth) / 2, screenHeight / 2 + 30, scoreSize, WHITE);

            const char* restartText = "Press SPACE to restart";
            int restartSize = 22;
            int restartWidth = MeasureText(restartText, restartSize);
            DrawText(restartText, (screenWidth - restartWidth) / 2, screenHeight / 2 + 80, restartSize, GRAY);
        }

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