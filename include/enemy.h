#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include <stdbool.h>

typedef enum
{
    ENEMY_IDLE,
    ENEMY_CHASING,
    ENEMY_ATTACKING,
    ENEMY_HURT,
    ENEMY_DYING,
    ENEMY_VICTORY
} EnemyState;

typedef struct
{
    int maxHealth;
    int damage;
    float moveSpeed;
    float detectionRange;
    float attackRange;
    float attackCooldown;
    float attackWindup;
} LevelEnemyConfig;

typedef struct
{
    Texture2D idle;
    Texture2D walk;
    Texture2D attack;
    Texture2D hurt;
    Texture2D death;
    Texture2D victory;
} ZombieAssets;

typedef struct
{
    Rectangle body;
    Rectangle attackBox;
    EnemyState state;

    int health;
    int maxHealth;
    int damage;

    float moveSpeed;
    float detectionRange;
    float attackRange;
    float attackCooldown;
    float attackCooldownTimer;
    float attackWindup;
    float attackTimer;
    float hurtTimer;
    float deathTimer;
    float deathFinalPoseTimer;

    float frameSize;
    float drawScale;
    float drawWidth;
    float drawHeight;
    float groundY;

    int animationFrame;
    int deathFrame;
    float animationTimer;

    bool facingRight;
    bool damageApplied;
    bool attackWindowOpened;
    bool active;
    bool deathFinished;
} ZombieEnemy;

LevelEnemyConfig GetLevelEnemyConfig(int level);

bool LoadZombieAssets(ZombieAssets *assets);
bool ZombieAssetsAreValid(const ZombieAssets *assets);
bool ZombieAssetLayoutIsValid(const ZombieAssets *assets);
void UnloadZombieAssets(ZombieAssets *assets);

ZombieEnemy CreateZombieEnemy(
    float bodyX,
    float groundY,
    float playerStandingDrawHeight,
    float zombieFrameSize,
    LevelEnemyConfig config
);

void ResetZombieEnemy(
    ZombieEnemy *enemy,
    float bodyX,
    float groundY,
    float playerStandingDrawHeight,
    float zombieFrameSize,
    LevelEnemyConfig config
);

void UpdateZombieEnemy(
    ZombieEnemy *enemy,
    Rectangle playerBody,
    bool playerIsAboveEnemy,
    bool playerAlive,
    bool allowAI,
    float dt,
    Sound telegraphSound,
    bool telegraphSoundReady
);

void MarkZombieAttackResolved(ZombieEnemy *enemy);

void DamageZombie(
    ZombieEnemy *enemy,
    int damage,
    bool playerIsLeft
);

void ForceZombieVictory(ZombieEnemy *enemy);
void ForceZombieCorpse(ZombieEnemy *enemy);
void UpdateZombieVictory(ZombieEnemy *enemy, float dt);

float GetZombieHorizontalGap(
    const ZombieEnemy *enemy,
    Rectangle playerBody
);

void DrawZombieEnemy(
    const ZombieEnemy *enemy,
    const ZombieAssets *assets,
    bool showHitboxes
);

#endif
