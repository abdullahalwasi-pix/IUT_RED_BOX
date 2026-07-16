#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include <stdbool.h>

typedef struct
{
    Texture2D idle;
    Texture2D run;
    Texture2D jump;
    Texture2D attack;
    Texture2D hurt;
    Texture2D death;
    Texture2D victory;

    float idleFrameWidth;
    float runFrameWidth;
    float jumpFrameWidth;
    float attackFrameWidth;
    float hurtFrameWidth;
    float deathFrameWidth;
    float victoryFrameWidth;
} PlayerAssets;

typedef struct
{
    float x;
    float previousX;
    float feetY;
    float groundY;
    float verticalVelocity;

    int health;

    bool isRunning;
    bool wasRunning;
    bool isSprinting;
    bool wasSprinting;
    bool isJumping;
    bool wasJumping;
    bool isSprintJump;
    bool isAttacking;
    bool facingRight;

    bool isHurt;
    bool isDying;
    bool deathFinished;

    bool attackLungeApplied;
    bool enemyHitThisAttack;
    bool specialAttack;
    bool attackActive;
    bool attackImpactEvent;

    int comboAttackCount;
    int currentFrame;
    int deathFrame;

    float comboResetTimer;
    float frameTimer;
    float hurtTimer;
    float deathTimer;
    float deathFinalPoseTimer;
    float victoryTimer;
    float invulnerabilityTimer;

    float hurtAnchorCenterX;
    float deathAnchorCenterX;
    float deathAnchorFeetY;

    Texture2D currentTexture;
    float currentFrameWidth;
    float currentFrameHeight;
    float drawScale;
    float frameDuration;
    int totalFrames;

    float drawX;
    float drawY;
    float drawWidth;
    float drawHeight;
    float referenceDrawWidth;
    float referenceDrawHeight;

    Rectangle body;
    Rectangle attackBox;
} Player;

bool LoadPlayerAssets(PlayerAssets *assets);
bool PlayerAssetsAreValid(const PlayerAssets *assets);
bool PlayerEndStateAssetLayoutIsValid(const PlayerAssets *assets);
void UnloadPlayerAssets(PlayerAssets *assets);

void InitPlayer(Player *player, const PlayerAssets *assets, float groundY);
void ResetPlayer(Player *player, const PlayerAssets *assets, float groundY);

void UpdatePlayer(
    Player *player,
    const PlayerAssets *assets,
    float dt,
    bool gameplayEnabled,
    bool victoryMode,
    Sound swordSwingSound,
    bool swordSwingSoundReady
);

void RefreshPlayerGeometry(Player *player, const PlayerAssets *assets);

void ApplyDamageToPlayer(
    Player *player,
    const PlayerAssets *assets,
    int damage
);

void StartPlayerVictory(Player *player);
void LockPlayerDeathFinalPose(Player *player);

int GetPlayerAttackDamage(const Player *player);
float GetPlayerAttackGapLimit(const Player *player);

void DrawPlayer(const Player *player, bool showHitboxes);

#endif
