#include "enemy.h"
#include <math.h>

// Single definition of the spawn-point array declared extern in enemy.h.
Vector2 g_enemySpawnPoints[4];

void InitEnemySpawnPoints(int screenWidth, int screenHeight, float groundLineY)
{
    float midX = screenWidth / 2.0f - ENEMY_DRAW_SIZE / 2.0f;
    float midY = (groundLineY + screenHeight) / 2.0f - ENEMY_DRAW_SIZE / 2.0f;

    g_enemySpawnPoints[0] = (Vector2){ midX, groundLineY };                    // top
    g_enemySpawnPoints[1] = (Vector2){ midX, screenHeight - ENEMY_DRAW_SIZE }; // bottom
    g_enemySpawnPoints[2] = (Vector2){ 0, midY };                              // left
    g_enemySpawnPoints[3] = (Vector2){ screenWidth - ENEMY_DRAW_SIZE, midY };  // right
}

// Loads the shared sprite sheets once. Every Enemy instance draws using
// these same textures - only their per-instance state (position, HP,
// current frame, etc.) differs between the 3 enemies.
void LoadEnemyTextures(EnemyTextures* tex)
{
    tex->texWalk = LoadTexture("resources/enemy_walk.png");   // 6 frames
    tex->texIdle = LoadTexture("resources/enemy_idle.png");   // 4 frames
    tex->texAttack = LoadTexture("resources/enemy_attack.png"); // 4 frames
    tex->texHurt = LoadTexture("resources/enemy_hurt.png");   // 4 frames
    tex->texDeath = LoadTexture("resources/enemy_death.png");  // 8 frames
}

void UnloadEnemyTextures(EnemyTextures* tex)
{
    UnloadTexture(tex->texWalk);
    UnloadTexture(tex->texIdle);
    UnloadTexture(tex->texAttack);
    UnloadTexture(tex->texHurt);
    UnloadTexture(tex->texDeath);
}

void InitEnemy(Enemy* e, Vector2 startPos)
{
    e->currentFrame = 0;
    e->frameTimer = 0.0f;

    e->pos = startPos;
    e->facingLeft = false;
    e->state = ENEMY_IDLE;

    e->hp = ENEMY_MAX_HP;
    e->maxHp = ENEMY_MAX_HP;

    e->attackCooldown = 0.0f;
    e->deathHoldTimer = 0.0f;
    e->respawnTimer = 0.0f;
    e->damageAppliedThisSwing = false;
}

// Rectangle covering the enemy's visible body, for collision checks -
// inset from the full sprite frame to exclude the transparent padding
// around the character (see the ENEMY_HITBOX_* constants in enemy.h).
Rectangle EnemyBounds(const Enemy* e)
{
    return (Rectangle) {
        e->pos.x + ENEMY_HITBOX_INSET_X,
            e->pos.y + ENEMY_HITBOX_INSET_TOP,
            ENEMY_HITBOX_WIDTH,
            ENEMY_HITBOX_HEIGHT
    };
}

Vector2 EnemyCenter(const Enemy* e)
{
    return (Vector2) { e->pos.x + ENEMY_DRAW_SIZE / 2.0f, e->pos.y + ENEMY_DRAW_SIZE / 2.0f };
}

// Applies damage and switches to the hurt animation, unless already dead.
// Returns true only on the hit that brings hp to 0 (the actual kill),
// so main.c can count kills without a separate state check.
bool EnemyTakeDamage(Enemy* e, int amount)
{
    if (e->state == ENEMY_DEAD) return false;

    e->hp -= amount;
    if (e->hp <= 0)
    {
        e->hp = 0;
        e->state = ENEMY_DEAD;
        e->currentFrame = 0;
        e->frameTimer = 0.0f;
        e->deathHoldTimer = 0.0f;
        e->respawnTimer = 0.0f;
        return true;
    }

    e->state = ENEMY_HURT;
    e->currentFrame = 0;
    e->frameTimer = 0.0f;
    return false;
}

// Picks a spawn point from g_enemySpawnPoints, avoiding any point too
// close to another currently-alive enemy so two enemies don't respawn
// stacked on each other. Falls back to a random point if every point is
// too close to something (e.g. with few enemies this should be rare).
static Vector2 PickRespawnPoint(const Enemy* self, Enemy* allEnemies, int allCount)
{
    // Try a handful of random picks first, preferring one that's clear.
    for (int attempt = 0; attempt < 8; attempt++)
    {
        Vector2 candidate = g_enemySpawnPoints[GetRandomValue(0, 3)];
        bool tooClose = false;

        for (int i = 0; i < allCount; i++)
        {
            Enemy* other = &allEnemies[i];
            if (other == self || other->state == ENEMY_DEAD) continue;

            float dx = other->pos.x - candidate.x;
            float dy = other->pos.y - candidate.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < ENEMY_MIN_SPAWN_SEPARATION) { tooClose = true; break; }
        }

        if (!tooClose) return candidate;
    }

    // Nothing clear after several tries - just use a random point anyway.
    return g_enemySpawnPoints[GetRandomValue(0, 3)];
}

// AI: walk toward the player until in attack range, then attack on a
// cooldown. Handles its own animation stepping and, once dead, the
// respawn timer/position reset. Takes a non-const Player* because a
// successful attack calls PlayerTakeDamage, which mutates it.
// allEnemies/allCount let a respawning enemy avoid other alive enemies'
// positions when picking a new spawn point.
void UpdateEnemy(Enemy* e, Player* player, float dt, int screenWidth, int screenHeight, float groundLineY,
    Enemy* allEnemies, int allCount)
{
    // ---- Dead: wait, then respawn at a spawn point clear of other enemies ----
    if (e->state == ENEMY_DEAD)
    {
        // Let the death animation itself play out and hold briefly first.
        if (e->currentFrame >= ENEMY_DEATH_LAST_FRAME)
        {
            e->deathHoldTimer += dt;
            if (e->deathHoldTimer >= ENEMY_DEATH_HOLD)
            {
                e->respawnTimer += dt;
                if (e->respawnTimer >= ENEMY_RESPAWN_TIME)
                {
                    e->pos = PickRespawnPoint(e, allEnemies, allCount);
                    e->hp = e->maxHp;
                    e->state = ENEMY_IDLE;
                    e->currentFrame = 0;
                    e->frameTimer = 0.0f;
                    e->attackCooldown = 0.0f;
                }
            }
        }
        else
        {
            // Still playing the death animation - step through it here
            // since the shared frame-advance block below only runs for
            // non-dead states.
            e->frameTimer += dt;
            if (e->frameTimer >= ENEMY_FRAME_SPEED)
            {
                e->frameTimer = 0.0f;
                e->currentFrame++;
                if (e->currentFrame > ENEMY_DEATH_LAST_FRAME) e->currentFrame = ENEMY_DEATH_LAST_FRAME; // hold on last frame
            }
        }
        return; // no movement/AI/collision while dead
    }

    // ---- AI: chase the player, attack when close enough ----
    // While hurt, the enemy is stunned - no chasing, no attacking. This
    // is what stops the bug where the enemy could land its own attack
    // on the same frame the player's hit put it into ENEMY_HURT.
    if (e->state == ENEMY_HURT)
    {
        const int hurtFrameCount = 4;

        e->frameTimer += dt;
        if (e->frameTimer >= ENEMY_FRAME_SPEED)
        {
            e->frameTimer = 0.0f;
            e->currentFrame++;
            if (e->currentFrame >= hurtFrameCount)
            {
                e->state = ENEMY_IDLE;
                e->currentFrame = 0;
            }
        }
        return; // skip movement/attack logic entirely while stunned
    }

    Vector2 toPlayer = { PlayerCenter(player).x - EnemyCenter(e).x, PlayerCenter(player).y - EnemyCenter(e).y };
    float dist = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);

    // "In range" just means the two characters' actual hitboxes are
    // touching/overlapping - no extra reach margin, matching what the
    // F1 debug boxes show.
    bool inRange = CheckCollisionRecs(EnemyBounds(e), PlayerBounds(player));

    if (e->attackCooldown > 0.0f) e->attackCooldown -= dt;

    if (inRange)
    {
        // Close enough - stop moving and attack if the cooldown is up.
        if (e->attackCooldown <= 0.0f && e->state != ENEMY_ATTACK)
        {
            e->state = ENEMY_ATTACK;
            e->currentFrame = 0;
            e->frameTimer = 0.0f;
            e->attackCooldown = ENEMY_ATTACK_COOLDOWN;
            e->damageAppliedThisSwing = false; // new swing, hasn't hit yet
        }
        else if (e->state != ENEMY_ATTACK)
        {
            e->state = ENEMY_IDLE;
        }

        // Damage only applies during the active window of the swing
        // (frames 1-2), and only once per swing - same windowed-hit
        // pattern as the player's melee in main.c, instead of dealing
        // damage the instant the attack starts.
        if (EnemyAttackIsActive(e) && !e->damageAppliedThisSwing)
        {
            PlayerTakeDamage(player, ENEMY_DAMAGE);
            e->damageAppliedThisSwing = true;
        }
    }
    else
    {
        // Walk toward the player.
        if (dist > 0.0001f)
        {
            e->pos.x += (toPlayer.x / dist) * ENEMY_SPEED * dt;
            e->pos.y += (toPlayer.y / dist) * ENEMY_SPEED * dt;
        }
        e->facingLeft = (toPlayer.x < 0);
        e->state = ENEMY_WALK;
    }

    // Keep the enemy within the same walkable area as the player.
    if (e->pos.x < 0) e->pos.x = 0;
    if (e->pos.x > screenWidth - ENEMY_DRAW_SIZE) e->pos.x = screenWidth - ENEMY_DRAW_SIZE;
    if (e->pos.y < groundLineY) e->pos.y = groundLineY;
    if (e->pos.y > screenHeight - ENEMY_DRAW_SIZE) e->pos.y = screenHeight - ENEMY_DRAW_SIZE;

    // ---- Advance animation ----
    // ENEMY_HURT is handled entirely in its own early-return block above,
    // so only ENEMY_ATTACK counts as a one-shot animation here.
    bool busy = (e->state == ENEMY_ATTACK);
    int frameCount = (e->state == ENEMY_WALK) ? 6 : 4;

    e->frameTimer += dt;
    if (e->frameTimer >= ENEMY_FRAME_SPEED)
    {
        e->frameTimer = 0.0f;
        e->currentFrame++;

        if (e->currentFrame >= frameCount)
        {
            if (busy) // attack finished -> back to normal AI state
            {
                e->state = ENEMY_IDLE;
                e->currentFrame = 0;
            }
            else
            {
                e->currentFrame = 0; // idle/walk loop
            }
        }
    }
}

// True only during the "active hit" window of the attack animation
// (frames 1 and 2 of the 4-frame sheet) - mirrors PlayerAttackIsActive so
// neither side deals damage the instant an attack starts.
bool EnemyAttackIsActive(const Enemy* e)
{
    return (e->state == ENEMY_ATTACK) &&
        (e->currentFrame >= ENEMY_ATTACK_WINDOW_START && e->currentFrame <= ENEMY_ATTACK_WINDOW_END);
}

void DrawEnemy(const Enemy* e, const EnemyTextures* tex)
{
    // While dead, the death animation plays using the same frame-count
    // logic as the death-hold state above (8 frames, held on the last).
    Texture2D activeTex = tex->texIdle;
    int frameCount = 4;
    switch (e->state)
    {
    case ENEMY_WALK:   activeTex = tex->texWalk;   frameCount = 6; break;
    case ENEMY_ATTACK: activeTex = tex->texAttack; frameCount = 4; break;
    case ENEMY_HURT:   activeTex = tex->texHurt;   frameCount = 4; break;
    case ENEMY_DEAD:   activeTex = tex->texDeath;  frameCount = ENEMY_DEATH_FRAME_COUNT; break;
    case ENEMY_IDLE:
    default:           activeTex = tex->texIdle;   frameCount = 4; break;
    }
    int frameWidth = activeTex.width / frameCount;

    Rectangle sourceRec = {
        (float)(e->currentFrame * frameWidth),
        0.0f,
        e->facingLeft ? -(float)frameWidth : (float)frameWidth,
        (float)ENEMY_FRAME_HEIGHT
    };
    Rectangle destRec = { e->pos.x, e->pos.y, ENEMY_DRAW_SIZE, ENEMY_DRAW_SIZE };
    DrawTexturePro(activeTex, sourceRec, destRec, (Vector2) { 0, 0 }, 0.0f, WHITE);

    // Small health bar above the enemy's head, only while alive.
    if (e->state != ENEMY_DEAD)
    {
        float pct = (float)e->hp / (float)e->maxHp;
        int barW = (int)ENEMY_DRAW_SIZE;
        int barX = (int)e->pos.x;
        int barY = (int)e->pos.y - 10;
        DrawRectangle(barX, barY, barW, 6, DARKGRAY);
        DrawRectangle(barX, barY, (int)(barW * pct), 6, RED);
        DrawRectangleLines(barX, barY, barW, 6, BLACK);
    }
}