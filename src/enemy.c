#include "enemy.h"
#include "config.h"
#include <math.h>

static float ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}

static bool TextureUsesEqualSquareFrames(
    Texture2D texture,
    int frameCount,
    int frameSize
)
{
    return texture.height == frameSize &&
           texture.width == frameSize * frameCount;
}

static void SetZombieState(
    ZombieEnemy *enemy,
    EnemyState newState
)
{
    if (enemy == 0 || enemy->state == newState)
    {
        return;
    }

    enemy->state = newState;
    enemy->animationFrame = 0;
    enemy->animationTimer = 0.0f;

    if (newState == ENEMY_ATTACKING)
    {
        enemy->attackTimer = 0.0f;
        enemy->damageApplied = false;
        enemy->attackWindowOpened = false;
    }
    else if (newState == ENEMY_HURT)
    {
        enemy->hurtTimer = 0.0f;
    }
    else if (newState == ENEMY_DYING)
    {
        enemy->deathTimer = 0.0f;
        enemy->deathFinalPoseTimer = 0.0f;
        enemy->deathFrame = 0;
        enemy->animationFrame = 0;
        enemy->damageApplied = true;
        enemy->attackWindowOpened = false;
        enemy->deathFinished = false;
    }
}

static Rectangle GetZombieAttackBox(
    const ZombieEnemy *enemy
)
{
    float attackWidth =
        enemy->body.height * 0.54f;

    float attackHeight =
        enemy->body.height * 0.40f;

    Rectangle box = {
        0.0f,
        enemy->body.y + enemy->body.height * 0.32f,
        attackWidth,
        attackHeight
    };

    box.x = enemy->facingRight
        ? enemy->body.x + enemy->body.width - 1.25f
        : enemy->body.x - box.width + 1.25f;

    return box;
}

static void AdvanceLoopingAnimation(
    ZombieEnemy *enemy,
    int frameCount,
    float frameDuration,
    float dt
)
{
    enemy->animationTimer += dt;

    while (enemy->animationTimer >= frameDuration)
    {
        enemy->animationTimer -= frameDuration;

        enemy->animationFrame =
            (enemy->animationFrame + 1) %
            frameCount;
    }
}

static Texture2D GetZombieTexture(
    const ZombieAssets *assets,
    EnemyState state
)
{
    switch (state)
    {
        case ENEMY_CHASING:
            return assets->walk;

        case ENEMY_ATTACKING:
            return assets->attack;

        case ENEMY_HURT:
            return assets->hurt;

        case ENEMY_DYING:
            return assets->death;

        case ENEMY_VICTORY:
            return assets->victory;

        case ENEMY_IDLE:
        default:
            return assets->idle;
    }
}

static Rectangle GetZombieSourceRectangle(
    const ZombieEnemy *enemy
)
{
    int renderFrame = enemy->animationFrame;

    if (
        enemy->state == ENEMY_DYING ||
        enemy->health <= 0
    )
    {
        renderFrame = enemy->deathFinished
            ? ZOMBIE_DEATH_HOLD_FRAME
            : enemy->deathFrame;
    }

    float frameX =
        (float)renderFrame *
        enemy->frameSize;

    
    if (enemy->state == ENEMY_DYING || enemy->health <= 0)
    {
        return (Rectangle){
            frameX,
            0.0f,
            enemy->frameSize,
            enemy->frameSize
        };
    }

    if (enemy->facingRight)
    {
        return (Rectangle){
            frameX,
            0.0f,
            enemy->frameSize,
            enemy->frameSize
        };
    }

    return (Rectangle){
        frameX + enemy->frameSize,
        0.0f,
        -enemy->frameSize,
        enemy->frameSize
    };
}

static Rectangle GetZombieDestinationRectangle(
    const ZombieEnemy *enemy
)
{
    float bodyCenterX =
        enemy->body.x +
        enemy->body.width * 0.5f;

    float drawX =
        bodyCenterX -
        enemy->drawWidth * 0.5f;

    float drawY =
        enemy->groundY -
        enemy->drawHeight *
        ZOMBIE_BASELINE_RATIO;

    return (Rectangle){
        drawX,
        drawY,
        enemy->drawWidth,
        enemy->drawHeight
    };
}

static void DrawZombieHealthBar(
    const ZombieEnemy *enemy
)
{
    if (
        enemy->state == ENEMY_DYING ||
        enemy->state == ENEMY_VICTORY ||
        enemy->health <= 0
    )
    {
        return;
    }

    Rectangle spriteDestination =
        GetZombieDestinationRectangle(enemy);

    float barWidth = 122.0f;
    float barHeight = 10.0f;

    float ratio =
        (float)enemy->health /
        (float)enemy->maxHealth;

    Rectangle back = {
        spriteDestination.x +
            spriteDestination.width * 0.5f -
            barWidth * 0.5f,
        spriteDestination.y - 19.0f,
        barWidth,
        barHeight
    };

    Rectangle fill = {
        back.x + 2.0f,
        back.y + 2.0f,
        (back.width - 4.0f) * ratio,
        back.height - 4.0f
    };

    DrawRectangleRounded(
        back,
        0.35f,
        8,
        Fade(BLACK, 0.82f)
    );

    DrawRectangleRounded(
        fill,
        0.35f,
        8,
        ratio > 0.50f
            ? LIME
            : (
                ratio > 0.22f
                    ? ORANGE
                    : RED
            )
    );

    DrawText(
        "ZOMBIE STUDENT",
        (int)(back.x + 5.0f),
        (int)(back.y - 16.0f),
        12,
        RAYWHITE
    );
}

LevelEnemyConfig GetLevelEnemyConfig(int level)
{
    switch (level)
    {
        case 1:
            return (LevelEnemyConfig){
                300,
                6,
                38.0f,
                1000.0f,
                17.0f,
                1.80f,
                0.60f
            };

        case 2:
            return (LevelEnemyConfig){
                90,
                10,
                52.0f,
                330.0f,
                17.0f,
                1.45f,
                0.50f
            };

        case 3:
            return (LevelEnemyConfig){
                110,
                12,
                60.0f,
                350.0f,
                18.0f,
                1.30f,
                0.46f
            };

        case 4:
            return (LevelEnemyConfig){
                130,
                14,
                68.0f,
                380.0f,
                20.0f,
                1.18f,
                0.42f
            };

        case 5:
            return (LevelEnemyConfig){
                155,
                17,
                77.0f,
                410.0f,
                22.0f,
                1.05f,
                0.38f
            };

        case 6:
            return (LevelEnemyConfig){
                185,
                20,
                88.0f,
                450.0f,
                24.0f,
                0.92f,
                0.34f
            };

        case 7:
        default:
            return (LevelEnemyConfig){
                225,
                24,
                100.0f,
                500.0f,
                26.0f,
                0.80f,
                0.30f
            };
    }
}

bool LoadZombieAssets(ZombieAssets *assets)
{
    if (assets == 0)
    {
        return false;
    }

    *assets = (ZombieAssets){0};

    assets->idle = LoadTexture(
        "../assets/enemy/zombie_student/idle.png"
    );

    assets->walk = LoadTexture(
        "../assets/enemy/zombie_student/walk.png"
    );

    assets->attack = LoadTexture(
        "../assets/enemy/zombie_student/attack.png"
    );

    assets->hurt = LoadTexture(
        "../assets/enemy/zombie_student/hurt.png"
    );

    assets->death = LoadTexture(
        "../assets/enemy/zombie_student/death.png"
    );

    assets->victory = LoadTexture(
        "../assets/enemy/zombie_student/victory.png"
    );

    if (!ZombieAssetsAreValid(assets))
    {
        return false;
    }

    SetTextureFilter(
        assets->idle,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureFilter(
        assets->walk,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureFilter(
        assets->attack,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureFilter(
        assets->hurt,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureFilter(
        assets->death,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureFilter(
        assets->victory,
        TEXTURE_FILTER_BILINEAR
    );

    return true;
}

bool ZombieAssetsAreValid(const ZombieAssets *assets)
{
    if (assets == 0)
    {
        return false;
    }

    return IsTextureValid(assets->idle) &&
           IsTextureValid(assets->walk) &&
           IsTextureValid(assets->attack) &&
           IsTextureValid(assets->hurt) &&
           IsTextureValid(assets->death) &&
           IsTextureValid(assets->victory);
}

bool ZombieAssetLayoutIsValid(
    const ZombieAssets *assets
)
{
    if (assets == 0)
    {
        return false;
    }

    int frameSize = assets->idle.height;

    if (frameSize <= 0)
    {
        return false;
    }

    return TextureUsesEqualSquareFrames(
               assets->idle,
               ZOMBIE_IDLE_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->walk,
               ZOMBIE_WALK_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->attack,
               ZOMBIE_ATTACK_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->hurt,
               ZOMBIE_HURT_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->death,
               ZOMBIE_DEATH_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->victory,
               ZOMBIE_VICTORY_FRAMES,
               frameSize
           );
}

void UnloadZombieAssets(ZombieAssets *assets)
{
    if (assets == 0)
    {
        return;
    }

    if (IsTextureValid(assets->idle))
    {
        UnloadTexture(assets->idle);
    }

    if (IsTextureValid(assets->walk))
    {
        UnloadTexture(assets->walk);
    }

    if (IsTextureValid(assets->attack))
    {
        UnloadTexture(assets->attack);
    }

    if (IsTextureValid(assets->hurt))
    {
        UnloadTexture(assets->hurt);
    }

    if (IsTextureValid(assets->death))
    {
        UnloadTexture(assets->death);
    }

    if (IsTextureValid(assets->victory))
    {
        UnloadTexture(assets->victory);
    }

    *assets = (ZombieAssets){0};
}

ZombieEnemy CreateZombieEnemy(
    float bodyX,
    float groundY,
    float playerStandingDrawHeight,
    float zombieFrameSize,
    LevelEnemyConfig config
)
{
    ZombieEnemy enemy = {0};

    enemy.frameSize = zombieFrameSize;

    enemy.drawHeight =
        playerStandingDrawHeight *
        ZOMBIE_PLAYER_HEIGHT_RATIO;

    enemy.drawWidth = enemy.drawHeight;

    enemy.drawScale =
        enemy.drawHeight /
        zombieFrameSize;

    enemy.groundY =
        groundY +
        ZOMBIE_GROUND_OFFSET;

    float bodyHeight =
        enemy.drawHeight *
        0.77f;

    float bodyWidth =
        enemy.drawHeight *
        0.38f;

    enemy.body = (Rectangle){
        bodyX,
        enemy.groundY - bodyHeight,
        bodyWidth,
        bodyHeight
    };

    enemy.state = ENEMY_IDLE;
    enemy.health = config.maxHealth;
    enemy.maxHealth = config.maxHealth;
    enemy.damage = config.damage;
    enemy.moveSpeed = config.moveSpeed;
    enemy.detectionRange = config.detectionRange;
    enemy.attackRange = config.attackRange;
    enemy.attackCooldown = config.attackCooldown;
    enemy.attackWindup = config.attackWindup;
    enemy.facingRight = false;
    enemy.active = true;
    enemy.deathFrame = 0;
    enemy.deathTimer = 0.0f;
    enemy.deathFinalPoseTimer = 0.0f;
    enemy.deathFinished = false;

    return enemy;
}

void ResetZombieEnemy(
    ZombieEnemy *enemy,
    float bodyX,
    float groundY,
    float playerStandingDrawHeight,
    float zombieFrameSize,
    LevelEnemyConfig config
)
{
    if (enemy == 0)
    {
        return;
    }

    *enemy = CreateZombieEnemy(
        bodyX,
        groundY,
        playerStandingDrawHeight,
        zombieFrameSize,
        config
    );
}

float GetZombieHorizontalGap(
    const ZombieEnemy *enemy,
    Rectangle playerBody
)
{
    if (enemy == 0)
    {
        return 0.0f;
    }

    if (
        playerBody.x + playerBody.width <
        enemy->body.x
    )
    {
        return enemy->body.x -
               (
                   playerBody.x +
                   playerBody.width
               );
    }

    if (
        enemy->body.x + enemy->body.width <
        playerBody.x
    )
    {
        return playerBody.x -
               (
                   enemy->body.x +
                   enemy->body.width
               );
    }

    return 0.0f;
}

void UpdateZombieEnemy(
    ZombieEnemy *enemy,
    Rectangle playerBody,
    bool playerIsAboveEnemy,
    bool playerAlive,
    bool allowAI,
    float dt,
    Sound telegraphSound,
    bool telegraphSoundReady
)
{
    if (enemy == 0 || !enemy->active)
    {
        return;
    }

    enemy->attackWindowOpened = false;

    if (enemy->attackCooldownTimer > 0.0f)
    {
        enemy->attackCooldownTimer -= dt;
    }

    float playerCenterX =
        playerBody.x +
        playerBody.width * 0.5f;

    float enemyCenterX =
        enemy->body.x +
        enemy->body.width * 0.5f;

    float distance =
        fabsf(
            playerCenterX -
            enemyCenterX
        );

    float horizontalGap =
        GetZombieHorizontalGap(
            enemy,
            playerBody
        );

    enemy->facingRight =
        playerCenterX >
        enemyCenterX;

    if (
        enemy->state == ENEMY_DYING ||
        enemy->health <= 0
    )
    {
        enemy->state = ENEMY_DYING;

        if (
            enemy->deathFrame <
            ZOMBIE_DEATH_HOLD_FRAME
        )
        {
            enemy->deathTimer += dt;

            while (
                enemy->deathTimer >=
                    ZOMBIE_DEATH_FRAME_TIME &&
                enemy->deathFrame <
                    ZOMBIE_DEATH_HOLD_FRAME
            )
            {
                enemy->deathTimer -=
                    ZOMBIE_DEATH_FRAME_TIME;

                enemy->deathFrame++;
            }
        }
        else
        {
            enemy->deathFrame =
                ZOMBIE_DEATH_HOLD_FRAME;

            enemy->deathFinalPoseTimer += dt;

            if (
                enemy->deathFinalPoseTimer >=
                ZOMBIE_DEATH_FINAL_POSE_TIME
            )
            {
                enemy->deathFinished = true;
            }
        }

        enemy->animationFrame =
            enemy->deathFinished
                ? ZOMBIE_DEATH_HOLD_FRAME
                : enemy->deathFrame;

        return;
    }

    if (enemy->state == ENEMY_VICTORY)
    {
        AdvanceLoopingAnimation(
            enemy,
            ZOMBIE_VICTORY_FRAMES,
            PLAYER_VICTORY_FRAME_TIME,
            dt
        );

        return;
    }

    if (!allowAI)
    {
        SetZombieState(
            enemy,
            ENEMY_IDLE
        );

        AdvanceLoopingAnimation(
            enemy,
            ZOMBIE_IDLE_FRAMES,
            0.145f,
            dt
        );

        return;
    }

    if (enemy->state == ENEMY_HURT)
    {
        enemy->hurtTimer += dt;

        int hurtFrame =
            (int)(
                enemy->hurtTimer /
                0.07f
            );

        if (hurtFrame >= ZOMBIE_HURT_FRAMES)
        {
            SetZombieState(
                enemy,
                ENEMY_IDLE
            );
        }
        else
        {
            enemy->animationFrame = hurtFrame;
        }

        return;
    }

    if (enemy->state == ENEMY_ATTACKING)
    {
        float previousAttackTimer =
            enemy->attackTimer;

        enemy->attackTimer += dt;

        enemy->attackBox =
            GetZombieAttackBox(enemy);

        float attackTotalDuration =
            enemy->attackWindup +
            0.34f;

        float progress = ClampFloat(
            enemy->attackTimer /
                attackTotalDuration,
            0.0f,
            0.9999f
        );

        enemy->animationFrame =
            (int)(
                progress *
                (float)ZOMBIE_ATTACK_FRAMES
            );

        if (
            previousAttackTimer <
                enemy->attackWindup &&
            enemy->attackTimer >=
                enemy->attackWindup &&
            !enemy->damageApplied
        )
        {
            enemy->attackWindowOpened = true;
        }

        if (
            enemy->attackTimer >=
                attackTotalDuration &&
            !enemy->attackWindowOpened
        )
        {
            SetZombieState(
                enemy,
                ENEMY_IDLE
            );

            enemy->attackCooldownTimer =
                enemy->attackCooldown;
        }

        return;
    }

    if (
        !playerIsAboveEnemy &&
        horizontalGap <= enemy->attackRange &&
        enemy->attackCooldownTimer <= 0.0f &&
        playerAlive
    )
    {
        SetZombieState(
            enemy,
            ENEMY_ATTACKING
        );

        enemy->attackBox =
            GetZombieAttackBox(enemy);

        if (telegraphSoundReady)
        {
            PlaySound(telegraphSound);
        }

        return;
    }

    if (
        distance <= enemy->detectionRange &&
        horizontalGap > enemy->attackRange &&
        playerAlive
    )
    {
        SetZombieState(
            enemy,
            ENEMY_CHASING
        );

        enemy->body.x +=
            (
                enemy->facingRight
                    ? 1.0f
                    : -1.0f
            ) *
            enemy->moveSpeed *
            dt;

        AdvanceLoopingAnimation(
            enemy,
            ZOMBIE_WALK_FRAMES,
            0.105f,
            dt
        );
    }
    else
    {
        SetZombieState(
            enemy,
            ENEMY_IDLE
        );

        AdvanceLoopingAnimation(
            enemy,
            ZOMBIE_IDLE_FRAMES,
            0.145f,
            dt
        );
    }

    enemy->body.x = ClampFloat(
        enemy->body.x,
        0.0f,
        (float)SCREEN_WIDTH -
            enemy->body.width
    );
}

void MarkZombieAttackResolved(
    ZombieEnemy *enemy
)
{
    if (enemy == 0)
    {
        return;
    }

    enemy->damageApplied = true;
    enemy->attackWindowOpened = false;
}

void DamageZombie(
    ZombieEnemy *enemy,
    int damage,
    bool playerIsLeft
)
{
    if (
        enemy == 0 ||
        !enemy->active ||
        enemy->state == ENEMY_DYING ||
        enemy->state == ENEMY_VICTORY
    )
    {
        return;
    }

    enemy->health -= damage;

    if (enemy->health <= 0)
    {
        enemy->health = 0;

        SetZombieState(
            enemy,
            ENEMY_DYING
        );

        return;
    }

    SetZombieState(
        enemy,
        ENEMY_HURT
    );

    enemy->body.x +=
        playerIsLeft
            ? 18.0f
            : -18.0f;
}

void ForceZombieVictory(
    ZombieEnemy *enemy
)
{
    if (
        enemy == 0 ||
        enemy->health <= 0
    )
    {
        return;
    }

    SetZombieState(
        enemy,
        ENEMY_VICTORY
    );
}

void ForceZombieCorpse(
    ZombieEnemy *enemy
)
{
    if (enemy == 0)
    {
        return;
    }

    enemy->health = 0;
    enemy->state = ENEMY_DYING;
    enemy->deathFrame =
        ZOMBIE_DEATH_HOLD_FRAME;

    enemy->animationFrame =
        ZOMBIE_DEATH_HOLD_FRAME;

    enemy->deathTimer = 0.0f;

    enemy->deathFinalPoseTimer =
        ZOMBIE_DEATH_FINAL_POSE_TIME;

    enemy->deathFinished = true;
    enemy->damageApplied = true;
    enemy->attackWindowOpened = false;
}

void UpdateZombieVictory(
    ZombieEnemy *enemy,
    float dt
)
{
    if (
        enemy == 0 ||
        enemy->state != ENEMY_VICTORY
    )
    {
        return;
    }

    AdvanceLoopingAnimation(
        enemy,
        ZOMBIE_VICTORY_FRAMES,
        PLAYER_VICTORY_FRAME_TIME,
        dt
    );
}

void DrawZombieEnemy(
    const ZombieEnemy *enemy,
    const ZombieAssets *assets,
    bool showHitboxes
)
{
    if (
        enemy == 0 ||
        assets == 0 ||
        !enemy->active
    )
    {
        return;
    }

    EnemyState renderState =
        enemy->state;

    if (enemy->health <= 0)
    {
        renderState = ENEMY_DYING;
    }

    Texture2D texture =
        GetZombieTexture(
            assets,
            renderState
        );

    Rectangle source =
        GetZombieSourceRectangle(enemy);

    Rectangle destination =
        GetZombieDestinationRectangle(enemy);

    DrawEllipse(
        (int)(
            enemy->body.x +
            enemy->body.width * 0.5f
        ),
        (int)(enemy->groundY + 2.0f),
        enemy->drawWidth * 0.19f,
        7.0f,
        Fade(BLACK, 0.28f)
    );

    if (
        enemy->state == ENEMY_ATTACKING &&
        !enemy->damageApplied
    )
    {
        DrawText(
            "!",
            (int)(
                enemy->body.x +
                enemy->body.width * 0.5f -
                4.0f
            ),
            (int)(enemy->body.y - 16.0f),
            24,
            RED
        );
    }

    Color tint = WHITE;

    if (
        enemy->state == ENEMY_HURT &&
        (
            (int)(
                enemy->hurtTimer *
                28.0f
            ) %
            2 ==
            0
        )
    )
    {
        tint = (Color){
            255,
            155,
            155,
            255
        };
    }

    DrawTexturePro(
        texture,
        source,
        destination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        tint
    );

    DrawZombieHealthBar(enemy);

    if (showHitboxes)
    {
        DrawRectangleLinesEx(
            enemy->body,
            2.0f,
            RED
        );

        if (
            enemy->state ==
            ENEMY_ATTACKING
        )
        {
            DrawRectangleLinesEx(
                enemy->attackBox,
                2.0f,
                ORANGE
            );
        }
    }
}