#include "player.h"
#include <math.h>

void InitPlayer(Player* p, Vector2 startPos)
{
    p->texWalk = LoadTexture("resources/player_walk.png");   // 6 frames
    p->texIdle = LoadTexture("resources/player_idle.png");   // 4 frames
    p->texAttack = LoadTexture("resources/player_attack.png"); // 4 frames
    p->texHurt = LoadTexture("resources/player_hurt.png");   // 4 frames
    p->texDeath = LoadTexture("resources/player_death.png");  // 8 frames
    p->texThrow = LoadTexture("resources/player_throw.png");  // 4 frames
    p->texRock = LoadTexture("resources/rock.png");

    p->currentFrame = 0;
    p->frameTimer = 0.0f;

    p->pos = startPos;
    p->facingLeft = false;
    p->isMoving = false;
    p->state = STATE_IDLE;

    p->deathHoldTimer = 0.0f;

    p->hp = PLAYER_MAX_HP;
    p->maxHp = PLAYER_MAX_HP;

    p->rockActive = false;
    p->rockPos = (Vector2){ 0 };
    p->rockVel = (Vector2){ 0 };
    p->rockStartPos = (Vector2){ 0 };
    p->throwAimDir = (Vector2){ 1.0f, 0.0f };
    p->isAiming = false;
}

// Returns the player's center point in world space - used for aiming,
// spawning the rock, and distance/collision checks against the enemy.
Vector2 PlayerCenter(const Player* p)
{
    return (Vector2) { p->pos.x + PLAYER_DRAW_SIZE / 2.0f, p->pos.y + PLAYER_DRAW_SIZE / 2.0f };
}

// Rectangle covering the player's visible body, for collision checks -
// inset from the full sprite frame to exclude the transparent padding
// around the character (see the PLAYER_HITBOX_* constants in player.h).
Rectangle PlayerBounds(const Player* p)
{
    return (Rectangle) {
        p->pos.x + PLAYER_HITBOX_INSET_X,
            p->pos.y + PLAYER_HITBOX_INSET_TOP,
            PLAYER_HITBOX_WIDTH,
            PLAYER_HITBOX_HEIGHT
    };
}

// Applies damage and switches to the hurt animation, unless already dead.
void PlayerTakeDamage(Player* p, int amount)
{
    if (p->state == STATE_DEATH) return; // already dead, ignore further hits

    p->hp -= amount;
    if (p->hp <= 0)
    {
        p->hp = 0;
        p->state = STATE_DEATH;
        p->deathHoldTimer = 0.0f;
    }
    else
    {
        p->state = STATE_HURT;
    }
    p->currentFrame = 0;
    p->frameTimer = 0.0f;
}

// Handles input, movement, the rock throw, and animation stepping.
// screenWidth/Height and groundLineY are passed in for bounds clamping.
void UpdatePlayer(Player* p, float dt, int screenWidth, int screenHeight, float groundLineY)
{
    p->isMoving = false;

    // busy = a one-shot animation (attack/hurt/death/throw) is currently playing
    bool busy = (p->state == STATE_ATTACK || p->state == STATE_HURT ||
        p->state == STATE_DEATH || p->state == STATE_THROW);

    // ---- Debug keys: force a one-shot animation ----
    // Only allowed when not already busy or aiming, so spamming the key
    // can't interrupt/restart an animation that's still mid-playback,
    // and can't fire an attack mid-throw-aim.
    if (!busy && !p->isAiming)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { p->state = STATE_ATTACK; p->currentFrame = 0; p->frameTimer = 0.0f; busy = true; }
        else if (IsKeyPressed(KEY_Y)) { p->state = STATE_HURT;  p->currentFrame = 0; p->frameTimer = 0.0f; busy = true; }
        else if (IsKeyPressed(KEY_T)) { p->state = STATE_DEATH; p->currentFrame = 0; p->frameTimer = 0.0f; p->deathHoldTimer = 0.0f; busy = true; }
    }

    // ---- Rock throw: hold right-click to aim, release to throw ----
    // rockActive being true blocks starting a new throw, so the player
    // can't spam rocks - only one can be in flight at a time.
    if (!busy && !p->rockActive)
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Vector2 mousePos = GetMousePosition();
            Vector2 center = PlayerCenter(p);
            Vector2 toMouse = { mousePos.x - center.x, mousePos.y - center.y };
            float len = sqrtf(toMouse.x * toMouse.x + toMouse.y * toMouse.y);
            if (len > 0.0001f) { toMouse.x /= len; toMouse.y /= len; }

            p->throwAimDir = toMouse;
            p->facingLeft = (toMouse.x < 0); // sprite still only flips left/right
            p->isAiming = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && p->isAiming)
        {
            p->isAiming = false;
            p->state = STATE_THROW;
            p->currentFrame = 0;
            p->frameTimer = 0.0f;

            Vector2 center = PlayerCenter(p);
            p->rockActive = true;
            p->rockPos = center;
            p->rockStartPos = center;
            p->rockVel.x = p->throwAimDir.x * ROCK_SPEED;
            p->rockVel.y = p->throwAimDir.y * ROCK_SPEED;
        }
    }

    // Movement only applies if not busy
    if (!busy)
    {
        if (IsKeyDown(KEY_D)) { p->pos.x += PLAYER_SPEED * dt; p->facingLeft = false; p->isMoving = true; }
        if (IsKeyDown(KEY_A)) { p->pos.x -= PLAYER_SPEED * dt; p->facingLeft = true;  p->isMoving = true; }
        if (IsKeyDown(KEY_S)) { p->pos.y += PLAYER_SPEED * dt; p->isMoving = true; }
        if (IsKeyDown(KEY_W)) { p->pos.y -= PLAYER_SPEED * dt; p->isMoving = true; }

        p->state = p->isMoving ? STATE_WALK : STATE_IDLE;
    }

    // Keep player within screen bounds, and no higher than the ground line
    if (p->pos.x < 0) p->pos.x = 0;
    if (p->pos.x > screenWidth - PLAYER_DRAW_SIZE) p->pos.x = screenWidth - PLAYER_DRAW_SIZE;
    if (p->pos.y < groundLineY) p->pos.y = groundLineY;
    if (p->pos.y > screenHeight - PLAYER_DRAW_SIZE) p->pos.y = screenHeight - PLAYER_DRAW_SIZE;

    // ---- Advance animation ----
    int frameCount = (p->state == STATE_WALK) ? 6 : (p->state == STATE_DEATH) ? 8 : 4;

    p->frameTimer += dt;
    if (p->frameTimer >= PLAYER_FRAME_SPEED)
    {
        p->frameTimer = 0.0f;
        p->currentFrame++;

        if (p->currentFrame >= frameCount)
        {
            // One-shot animations (attack/hurt/death/throw) stop instead of
            // looping. Idle/walk just loop back to frame 0.
            if (p->state == STATE_DEATH)
            {
                // Stay on the last death frame; the hold timer below is
                // what eventually resets back to idle.
                p->currentFrame = frameCount - 1;
            }
            else if (busy) // attack, hurt, or throw finished
            {
                // Reset currentFrame to 0 before switching state so the
                // next texture (idle) is never sampled with a leftover
                // frame index it doesn't have.
                p->state = STATE_IDLE;
                p->currentFrame = 0;
            }
            else
            {
                p->currentFrame = 0;
            }
        }
    }

    // Death holds on its last frame for a bit, then resets to idle.
    if (p->state == STATE_DEATH && p->currentFrame == frameCount - 1)
    {
        p->deathHoldTimer += dt;
        if (p->deathHoldTimer >= PLAYER_DEATH_HOLD)
        {
            p->deathHoldTimer = 0.0f;
            p->state = STATE_IDLE;
            p->currentFrame = 0;
            p->hp = p->maxHp; // simple respawn: full heal on reset
        }
    }

    // ---- Update rock projectile ----
    if (p->rockActive)
    {
        p->rockPos.x += p->rockVel.x * dt;
        p->rockPos.y += p->rockVel.y * dt;

        float traveled = sqrtf(
            (p->rockPos.x - p->rockStartPos.x) * (p->rockPos.x - p->rockStartPos.x) +
            (p->rockPos.y - p->rockStartPos.y) * (p->rockPos.y - p->rockStartPos.y)
        );

        bool offScreen = (p->rockPos.x < -32 || p->rockPos.x > screenWidth + 32 ||
            p->rockPos.y < -32 || p->rockPos.y > screenHeight + 32);

        if (offScreen || traveled >= ROCK_MAX_DISTANCE)
        {
            p->rockActive = false;
        }
    }
}

// True only during the "active hit" window of the attack animation
// (frames 2 and 3 of the 4-frame sheet) - this is when melee damage
// should actually apply, not the instant the click happens.
bool PlayerAttackIsActive(const Player* p)
{
    return (p->state == STATE_ATTACK) && (p->currentFrame == 1 || p->currentFrame == 2);
}

void DrawPlayer(const Player* p)
{
    Texture2D activeTex = p->texIdle;
    int frameCount = 4;
    switch (p->state)
    {
    case STATE_WALK:   activeTex = p->texWalk;   frameCount = 6; break;
    case STATE_ATTACK: activeTex = p->texAttack; frameCount = 4; break;
    case STATE_HURT:   activeTex = p->texHurt;   frameCount = 4; break;
    case STATE_DEATH:  activeTex = p->texDeath;  frameCount = 8; break;
    case STATE_THROW:  activeTex = p->texThrow;  frameCount = 4; break;
    case STATE_IDLE:
    default:           activeTex = p->texIdle;   frameCount = 4; break;
    }
    int frameWidth = activeTex.width / frameCount;

    Rectangle sourceRec = {
        (float)(p->currentFrame * frameWidth),
        0.0f,
        p->facingLeft ? -(float)frameWidth : (float)frameWidth,
        (float)PLAYER_FRAME_HEIGHT
    };
    Rectangle destRec = { p->pos.x, p->pos.y, PLAYER_DRAW_SIZE, PLAYER_DRAW_SIZE };
    DrawTexturePro(activeTex, sourceRec, destRec, (Vector2) { 0, 0 }, 0.0f, WHITE);

    // Aim line: visible the whole time right-click is held down, so the
    // player can see where the rock will go before releasing.
    if (p->isAiming)
    {
        Vector2 center = PlayerCenter(p);
        Vector2 lineEnd = { center.x + p->throwAimDir.x * 60.0f, center.y + p->throwAimDir.y * 60.0f };
        DrawLineEx(center, lineEnd, 2.0f, Fade(YELLOW, 0.8f));
    }

    // Rock projectile
    if (p->rockActive)
    {
        float rockDrawSize = p->texRock.width * ROCK_SCALE;
        Rectangle rockSource = { 0, 0, (float)p->texRock.width, (float)p->texRock.height };
        // rockPos tracks the CENTER of the rock, but DrawTexturePro
        // positions from the top-left corner, so shift back by half the
        // draw size to keep it centered on rockPos.
        Rectangle rockDest = {
            p->rockPos.x - rockDrawSize / 2.0f,
            p->rockPos.y - rockDrawSize / 2.0f,
            rockDrawSize, rockDrawSize
        };
        DrawTexturePro(p->texRock, rockSource, rockDest, (Vector2) { 0, 0 }, 0.0f, WHITE);
    }
}

// Simple top-left HUD health bar.
void DrawPlayerHealthBar(const Player* p)
{
    int barX = 10, barY = 45, barW = 200, barH = 20;
    float pct = (float)p->hp / (float)p->maxHp;

    DrawRectangle(barX, barY, barW, barH, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barW * pct), barH, RED);
    DrawRectangleLines(barX, barY, barW, barH, BLACK);
    DrawText(TextFormat("HP: %d / %d", p->hp, p->maxHp), barX + 5, barY + 2, 16, WHITE);
}

void UnloadPlayer(Player* p)
{
    UnloadTexture(p->texWalk);
    UnloadTexture(p->texIdle);
    UnloadTexture(p->texAttack);
    UnloadTexture(p->texHurt);
    UnloadTexture(p->texDeath);
    UnloadTexture(p->texThrow);
    UnloadTexture(p->texRock);
}