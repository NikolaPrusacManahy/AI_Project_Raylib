#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "player.h"

// Enemy animation states - separate from PlayerState since the enemy's
// behavior rules (AI-driven, not input-driven) are different enough that
// sharing one enum would just add confusing overlap.
typedef enum {
    ENEMY_IDLE,
    ENEMY_WALK,
    ENEMY_ATTACK,
    ENEMY_HURT,
    ENEMY_DEAD // dead but waiting to respawn (not the death animation itself)
} EnemyState;

typedef struct {
    Texture2D texWalk;
    Texture2D texIdle;
    Texture2D texAttack;
    Texture2D texHurt;
    Texture2D texDeath;

    int currentFrame;
    float frameTimer;

    Vector2 pos;
    bool facingLeft;
    EnemyState state;

    int hp;
    int maxHp;

    float attackCooldown; // counts down; enemy can attack again once it hits 0
    float deathHoldTimer; // how long the death animation has been finished
    float respawnTimer;   // counts up while dead, waiting to respawn

    // True once the current attack swing has already damaged the player,
    // so a multi-frame active window can't deal damage more than once
    // per swing. Reset whenever a new attack starts.
    bool damageAppliedThisSwing;
} Enemy;

#define ENEMY_SCALE        3.0f
#define ENEMY_FRAME_HEIGHT 32
#define ENEMY_FRAME_SPEED  0.1f
#define ENEMY_SPEED        190.0f // slightly slower than the player's 250
#define ENEMY_MAX_HP       50
#define ENEMY_DAMAGE       10
// How far beyond the two characters' actual hitboxes (see *_HITBOX_*
// above) the enemy can still reach to attack - NOT a raw center-to-center
// distance. Using the real hitboxes plus a small melee margin means this
// stays visually correct even if the sprite art or scale changes later,
// instead of an arbitrary center-distance number that has to be
// hand-tuned separately from how big the characters actually are.
#define ENEMY_ATTACK_REACH 2.0f
#define ENEMY_ATTACK_COOLDOWN 1.0f  // seconds between attacks while in range
#define ENEMY_DEATH_HOLD   0.6f     // seconds to hold on the last death frame
#define ENEMY_RESPAWN_TIME 2.0f     // seconds dead before respawning
#define ENEMY_DEATH_FRAME_COUNT 8
#define ENEMY_DEATH_LAST_FRAME (ENEMY_DEATH_FRAME_COUNT - 1)

// The enemy's 4-frame attack sheet only deals damage during frames 1-2
// (the middle of the swing) - same windowed-hit idea as the player's
// PlayerAttackIsActive, so neither side does "instant" undodgeable damage
// the moment an attack starts.
#define ENEMY_ATTACK_WINDOW_START 1
#define ENEMY_ATTACK_WINDOW_END   2

// Size of the enemy on screen, after scaling (sprites are square).
#define ENEMY_DRAW_SIZE (ENEMY_FRAME_HEIGHT * ENEMY_SCALE)

// Same padding problem as the player - the sprite sheet has transparent
// space around the character, so the hitbox is inset to the actual
// visible body (measured from the idle/walk frames' non-transparent
// pixels, in source-sprite pixels before scaling).
#define ENEMY_HITBOX_INSET_X (3.0f * ENEMY_SCALE)
#define ENEMY_HITBOX_INSET_TOP (5.0f * ENEMY_SCALE)
#define ENEMY_HITBOX_WIDTH  (22.0f * ENEMY_SCALE)
#define ENEMY_HITBOX_HEIGHT (27.0f * ENEMY_SCALE)

// Four fixed spawn points, one per screen edge. Declared here so main.c
// and enemy.c share the same array; defined once in enemy.c and filled
// in by InitEnemySpawnPoints once the screen size is known.
extern Vector2 g_enemySpawnPoints[4];

// ---- Functions (implemented in src/enemy.c) ----
void InitEnemySpawnPoints(int screenWidth, int screenHeight, float groundLineY);
void InitEnemy(Enemy* e, Vector2 startPos);
void UpdateEnemy(Enemy* e, Player* player, float dt, int screenWidth, int screenHeight, float groundLineY);
void DrawEnemy(const Enemy* e);
void UnloadEnemy(Enemy* e);

void EnemyTakeDamage(Enemy* e, int amount);
Vector2 EnemyCenter(const Enemy* e);
Rectangle EnemyBounds(const Enemy* e);
bool EnemyAttackIsActive(const Enemy* e);

#endif // ENEMY_H