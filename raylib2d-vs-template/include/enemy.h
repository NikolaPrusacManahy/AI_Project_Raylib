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

// Sprite sheets shared by every enemy instance - loaded once, not per
// enemy, since all 3 enemies use the same art. Kept separate from Enemy
// itself so adding more enemies never means more texture loads.
typedef struct {
    Texture2D texWalk;
    Texture2D texIdle;
    Texture2D texAttack;
    Texture2D texHurt;
    Texture2D texDeath;
} EnemyTextures;

typedef struct {
    int currentFrame;   // which frame of the current animation is showing
    float frameTimer;   // counts up toward ENEMY_FRAME_SPEED to advance currentFrame

    Vector2 pos;         // top-left corner of the sprite, in world coordinates
    bool facingLeft;     // true = sprite is flipped to face left
    EnemyState state;    // what the enemy is currently doing (idle/walk/attack/hurt/dead)

    int hp;    // current health
    int maxHp; // health when fully healed (used for the health bar percentage)

    float attackCooldown; // counts down; enemy can attack again once it hits 0
    float deathHoldTimer; // how long the death animation has been finished
    float respawnTimer;   // counts up while dead, waiting to respawn

    // True once the current attack swing has already damaged the player,
    // so a multi-frame active window can't deal damage more than once
    // per swing. Reset whenever a new attack starts.
    bool damageAppliedThisSwing;
} Enemy;

#define ENEMY_SCALE        3.0f   // how much bigger the 32x32 sprite is drawn (96x96 on screen)
#define ENEMY_FRAME_HEIGHT 32     // every spritesheet frame is 32px tall
#define ENEMY_FRAME_SPEED  0.1f   // seconds per animation frame
#define ENEMY_SPEED        190.0f // slightly slower than the player's 250
#define ENEMY_MAX_HP       50     // starting/maximum health
#define ENEMY_DAMAGE       10     // damage dealt to the player per successful hit
#define ENEMY_ATTACK_COOLDOWN 1.0f  // seconds between attacks while in range
#define ENEMY_DEATH_HOLD   0.6f     // seconds to hold on the last death frame
#define ENEMY_RESPAWN_TIME 2.0f     // seconds dead before respawning
#define ENEMY_DEATH_FRAME_COUNT 8
#define ENEMY_DEATH_LAST_FRAME (ENEMY_DEATH_FRAME_COUNT - 1)

// How many enemies exist at once, and how many total kills (across all of
// them) are needed to unlock the transition to the next map.
#define ENEMY_COUNT 3
#define KILLS_TO_UNLOCK_EXIT 7

// When picking a random respawn point, an enemy re-rolls if the chosen
// point is closer than this to another currently-alive enemy, so two
// enemies don't respawn stacked on top of each other.
#define ENEMY_MIN_SPAWN_SEPARATION 80.0f

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
void InitEnemySpawnPoints(int screenWidth, int screenHeight, float groundLineY); // fills in g_enemySpawnPoints - call once at startup
void LoadEnemyTextures(EnemyTextures* tex);   // loads the shared sprite sheets once
void UnloadEnemyTextures(EnemyTextures* tex); // frees them - call once before closing the window

void InitEnemy(Enemy* e, Vector2 startPos); // sets up one enemy at startPos with full health
// allEnemies/allCount are passed so a respawning enemy can avoid picking
// a spawn point too close to another enemy that's currently alive.
// allowRespawn gates whether a dead enemy is allowed to respawn at all -
// false once the kill target has been reached, so the enemy count winds
// down instead of respawning forever.
void UpdateEnemy(Enemy* e, Player* player, float dt, int screenWidth, int screenHeight, float groundLineY,
    Enemy* allEnemies, int allCount, bool allowRespawn);
void DrawEnemy(const Enemy* e, const EnemyTextures* tex); // draws the sprite and its health bar

// Returns true if this hit was the one that killed the enemy (hp reached
// 0), so callers can count kills without separately checking state.
bool EnemyTakeDamage(Enemy* e, int amount);
Vector2 EnemyCenter(const Enemy* e);      // the middle point of the enemy's sprite, in world coordinates
Rectangle EnemyBounds(const Enemy* e);    // the enemy's real (shrunk) hitbox, for collision checks
bool EnemyAttackIsActive(const Enemy* e); // true only during the frames where its swing can actually hit the player

#endif // ENEMY_H