#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"

// Player animation states
typedef enum {
    STATE_IDLE,
    STATE_WALK,
    STATE_ATTACK,
    STATE_HURT,
    STATE_DEATH,
    STATE_THROW
} PlayerState;

typedef struct {
    // ---- Textures ----
    // All sheets are 32px tall, frames laid out horizontally.
    Texture2D texWalk;
    Texture2D texIdle;
    Texture2D texAttack;
    Texture2D texHurt;
    Texture2D texDeath;
    Texture2D texThrow;
    Texture2D texRock; // single 16x16 sprite, not a sheet

    // ---- Animation ----
    int currentFrame;
    float frameTimer;

    // ---- Position / movement ----
    Vector2 pos;
    bool facingLeft;
    bool isMoving;
    PlayerState state;

    float deathHoldTimer; // counts up once death's last frame is reached

    // ---- Health ----
    int hp;
    int maxHp;

    // ---- Rock throw ----
    // Only one rock in flight at a time - a single set of fields is
    // enough (no array needed for this pass).
    bool rockActive;
    Vector2 rockPos;
    Vector2 rockVel;
    Vector2 rockStartPos; // used to measure travel distance
    Vector2 throwAimDir;  // direction locked in at the moment of throwing
    bool isAiming;         // true while right-click is held down
} Player;

// ---- Tunable constants ----
#define PLAYER_SCALE        3.0f
#define PLAYER_FRAME_HEIGHT 32
#define PLAYER_FRAME_SPEED  0.1f   // seconds per animation frame
#define PLAYER_SPEED        250.0f // pixels per second
#define PLAYER_MAX_HP       100
#define PLAYER_DEATH_HOLD   1.0f   // seconds to hold on the last death frame

#define ROCK_SPEED         600.0f
#define ROCK_MAX_DISTANCE  900.0f
#define ROCK_SCALE         2.0f
#define ROCK_DAMAGE        50

#define PLAYER_ATTACK_DAMAGE 25

// Size of the player on screen, after scaling (sprites are square).
#define PLAYER_DRAW_SIZE (PLAYER_FRAME_HEIGHT * PLAYER_SCALE)

// The sprite sheets have transparent padding around the character (for
// consistent frame alignment across the walk/attack/etc animations), so
// a hitbox sized to the full PLAYER_DRAW_SIZE is much bigger than the
// visible character - collisions would register well before the sprites
// actually look like they're touching. These insets (measured from the
// idle/walk frames' actual non-transparent pixels, in source-sprite
// pixels before scaling) shrink the hitbox down to the character's body.
#define PLAYER_HITBOX_INSET_X (3.0f * PLAYER_SCALE)  // left/right padding to trim
#define PLAYER_HITBOX_INSET_TOP (5.0f * PLAYER_SCALE)    // padding to trim off the top
#define PLAYER_HITBOX_WIDTH  (22.0f * PLAYER_SCALE)  // visible body width
#define PLAYER_HITBOX_HEIGHT (27.0f * PLAYER_SCALE)  // visible body height

// ---- Functions (implemented in src/player.c) ----
void InitPlayer(Player* p, Vector2 startPos);
void UpdatePlayer(Player* p, float dt, int screenWidth, int screenHeight, float groundLineY, bool allowExitRight);
void DrawPlayer(const Player* p);
void DrawPlayerHealthBar(const Player* p);
void UnloadPlayer(Player* p);

void PlayerTakeDamage(Player* p, int amount);
Vector2 PlayerCenter(const Player* p);
Rectangle PlayerBounds(const Player* p);
bool PlayerAttackIsActive(const Player* p);

// True once the player has died AND the death animation has fully played
// out (holding on its last frame) - the point at which main.c should
// freeze the game and show the death screen, rather than mid-animation.
bool PlayerDeathFinished(const Player* p);

#endif // PLAYER_H