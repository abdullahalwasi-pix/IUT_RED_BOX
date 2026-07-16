#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define CURRENT_LEVEL 1

#define PLAYER_MAX_HEALTH 100
#define PLAYER_SWORD_DAMAGE 25
#define PLAYER_SPECIAL_DAMAGE 50
#define COMBO_REQUIRED_ATTACKS 3
#define COMBO_RESET_TIME 0.90f
#define PLAYER_IDLE_SCALE 0.52f
#define PLAYER_RUN_SCALE 0.72f
#define PLAYER_JUMP_SCALE 0.52f
#define PLAYER_ATTACK_SCALE 0.78f
#define PLAYER_ATTACK_GROUND_OFFSET 70.0f
#define PLAYER_NORMAL_ATTACK_WIDTH 88.0f
#define PLAYER_SPECIAL_ATTACK_WIDTH 126.0f
#define PLAYER_NORMAL_ATTACK_HEIGHT 76.0f
#define PLAYER_SPECIAL_ATTACK_HEIGHT 94.0f
#define PLAYER_NORMAL_ATTACK_GAP 26.0f
#define PLAYER_SPECIAL_ATTACK_GAP 40.0f

#define PLAYER_HURT_FRAMES 3
#define PLAYER_DEATH_FRAMES 8
#define PLAYER_VICTORY_FRAMES 6
#define PLAYER_DEATH_HOLD_FRAME (PLAYER_DEATH_FRAMES - 1)
#define PLAYER_DEATH_FINAL_POSE_TIME 0.85f

#define PLAYER_HURT_SCALE 0.52f
#define PLAYER_DEATH_SCALE 0.52f
#define PLAYER_VICTORY_SCALE 0.52f

#define PLAYER_HURT_GROUND_OFFSET 0.0f
#define PLAYER_DEATH_GROUND_OFFSET 0.0f
#define PLAYER_VICTORY_GROUND_OFFSET 0.0f

#define PLAYER_HURT_FRAME_TIME 0.085f
#define PLAYER_DEATH_FRAME_TIME 0.110f
#define PLAYER_VICTORY_FRAME_TIME 0.140f
#define RESULT_PANEL_DELAY 3.00f

#define ZOMBIE_BASELINE_RATIO (226.0f / 256.0f)
#define ZOMBIE_PLAYER_HEIGHT_RATIO 0.68f
#define ZOMBIE_GROUND_OFFSET -48.0f
#define ZOMBIE_IDLE_FRAMES 8
#define ZOMBIE_WALK_FRAMES 8
#define ZOMBIE_ATTACK_FRAMES 8
#define ZOMBIE_HURT_FRAMES 4
#define ZOMBIE_DEATH_FRAMES 8
#define ZOMBIE_VICTORY_FRAMES 6
#define ZOMBIE_DEATH_HOLD_FRAME (ZOMBIE_DEATH_FRAMES - 1)
#define ZOMBIE_DEATH_FRAME_TIME 0.105f
#define ZOMBIE_DEATH_FINAL_POSE_TIME 0.85f

typedef enum {
    ENEMY_IDLE,
    ENEMY_CHASING,
    ENEMY_ATTACKING,
    ENEMY_HURT,
    ENEMY_DYING,
    ENEMY_VICTORY
} EnemyState;

typedef enum {
    GAME_PLAYING,
    GAME_VICTORY,
    GAME_DEFEAT
} GameState;

typedef struct {
    int maxHealth;
    int damage;
    float moveSpeed;
    float detectionRange;
    float attackRange;
    float attackCooldown;
    float attackWindup;
} LevelEnemyConfig;

typedef struct {
    Texture2D idle;
    Texture2D walk;
    Texture2D attack;
    Texture2D hurt;
    Texture2D death;
    Texture2D victory;
} ZombieAssets;

typedef struct {
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
    bool active;
    bool deathFinished;
} ZombieEnemy;

static float ClampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static LevelEnemyConfig GetLevelEnemyConfig(int level)
{
    switch (level)
    {
        case 1: return (LevelEnemyConfig){ 30,  6,  38.0f, 430.0f, 17.0f, 1.80f, 0.60f};
        case 2: return (LevelEnemyConfig){ 90, 10,  52.0f, 330.0f, 17.0f, 1.45f, 0.50f};
        case 3: return (LevelEnemyConfig){110, 12,  60.0f, 350.0f, 18.0f, 1.30f, 0.46f};
        case 4: return (LevelEnemyConfig){130, 14,  68.0f, 380.0f, 20.0f, 1.18f, 0.42f};
        case 5: return (LevelEnemyConfig){155, 17,  77.0f, 410.0f, 22.0f, 1.05f, 0.38f};
        case 6: return (LevelEnemyConfig){185, 20,  88.0f, 450.0f, 24.0f, 0.92f, 0.34f};
        case 7:
        default: return (LevelEnemyConfig){225, 24, 100.0f, 500.0f, 26.0f, 0.80f, 0.30f};
    }
}

static ZombieAssets LoadZombieAssets(void)
{
    ZombieAssets assets = {0};
    assets.idle = LoadTexture("../assets/enemy/zombie_student/idle.png");
    assets.walk = LoadTexture("../assets/enemy/zombie_student/walk.png");
    assets.attack = LoadTexture("../assets/enemy/zombie_student/attack.png");
    assets.hurt = LoadTexture("../assets/enemy/zombie_student/hurt.png");
    assets.death = LoadTexture("../assets/enemy/zombie_student/death.png");
    assets.victory = LoadTexture("../assets/enemy/zombie_student/victory.png");
    return assets;
}

static bool ZombieAssetsAreValid(const ZombieAssets *assets)
{
    return IsTextureValid(assets->idle) &&
           IsTextureValid(assets->walk) &&
           IsTextureValid(assets->attack) &&
           IsTextureValid(assets->hurt) &&
           IsTextureValid(assets->death) &&
           IsTextureValid(assets->victory);
}

static bool TextureUsesEqualSquareFrames(Texture2D texture, int frameCount, int frameSize)
{
    return texture.height == frameSize &&
           texture.width == frameSize * frameCount;
}

static bool PlayerEndStateAssetLayoutIsValid(
    Texture2D hurt,
    Texture2D death,
    Texture2D victory
)
{
    int frameSize = hurt.height;

    if (frameSize <= 0)
    {
        return false;
    }

    return TextureUsesEqualSquareFrames(
               hurt,
               PLAYER_HURT_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               death,
               PLAYER_DEATH_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               victory,
               PLAYER_VICTORY_FRAMES,
               frameSize
           );
}

static bool ZombieAssetLayoutIsValid(const ZombieAssets *assets)
{
    int frameSize = assets->idle.height;

    if (frameSize <= 0)
    {
        return false;
    }

    return TextureUsesEqualSquareFrames(assets->idle, ZOMBIE_IDLE_FRAMES, frameSize) &&
           TextureUsesEqualSquareFrames(assets->walk, ZOMBIE_WALK_FRAMES, frameSize) &&
           TextureUsesEqualSquareFrames(assets->attack, ZOMBIE_ATTACK_FRAMES, frameSize) &&
           TextureUsesEqualSquareFrames(assets->hurt, ZOMBIE_HURT_FRAMES, frameSize) &&
           TextureUsesEqualSquareFrames(assets->death, ZOMBIE_DEATH_FRAMES, frameSize) &&
           TextureUsesEqualSquareFrames(assets->victory, ZOMBIE_VICTORY_FRAMES, frameSize);
}

static void UnloadZombieAssets(ZombieAssets *assets)
{
    if (IsTextureValid(assets->idle)) UnloadTexture(assets->idle);
    if (IsTextureValid(assets->walk)) UnloadTexture(assets->walk);
    if (IsTextureValid(assets->attack)) UnloadTexture(assets->attack);
    if (IsTextureValid(assets->hurt)) UnloadTexture(assets->hurt);
    if (IsTextureValid(assets->death)) UnloadTexture(assets->death);
    if (IsTextureValid(assets->victory)) UnloadTexture(assets->victory);
}

static void SetEnemyState(ZombieEnemy *enemy, EnemyState newState)
{
    if (enemy->state == newState) return;

    enemy->state = newState;
    enemy->animationFrame = 0;
    enemy->animationTimer = 0.0f;

    if (newState == ENEMY_ATTACKING)
    {
        enemy->attackTimer = 0.0f;
        enemy->damageApplied = false;
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
        enemy->deathFinished = false;
    }
}

static ZombieEnemy CreateZombieEnemy(
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

static Rectangle GetEnemyAttackBox(const ZombieEnemy *enemy)
{
    float attackWidth = enemy->body.height * 0.54f;
    float attackHeight = enemy->body.height * 0.40f;

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
        enemy->animationFrame = (enemy->animationFrame + 1) % frameCount;
    }
}

static void DamageEnemy(
    ZombieEnemy *enemy,
    int damage,
    bool playerIsLeft
)
{
    if (!enemy->active || enemy->state == ENEMY_DYING || enemy->state == ENEMY_VICTORY)
    {
        return;
    }

    enemy->health -= damage;

    if (enemy->health <= 0)
    {
        enemy->health = 0;
        SetEnemyState(enemy, ENEMY_DYING);
        return;
    }

    SetEnemyState(enemy, ENEMY_HURT);
    enemy->body.x += playerIsLeft ? 18.0f : -18.0f;
}

static Texture2D GetEnemyTexture(
    const ZombieAssets *assets,
    EnemyState state
)
{
    switch (state)
    {
        case ENEMY_CHASING: return assets->walk;
        case ENEMY_ATTACKING: return assets->attack;
        case ENEMY_HURT: return assets->hurt;
        case ENEMY_DYING: return assets->death;
        case ENEMY_VICTORY: return assets->victory;
        case ENEMY_IDLE:
        default: return assets->idle;
    }
}

static Rectangle GetEnemySourceRectangle(const ZombieEnemy *enemy)
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

static Rectangle GetEnemyDestinationRectangle(const ZombieEnemy *enemy)
{
    float bodyCenterX = enemy->body.x + enemy->body.width * 0.5f;
    float drawX = bodyCenterX - enemy->drawWidth * 0.5f;
    float drawY = enemy->groundY - enemy->drawHeight * ZOMBIE_BASELINE_RATIO;

    return (Rectangle){
        drawX,
        drawY,
        enemy->drawWidth,
        enemy->drawHeight
    };
}

static void DrawEnemyHealthBar(const ZombieEnemy *enemy)
{
    if (enemy->state == ENEMY_DYING || enemy->state == ENEMY_VICTORY)
    {
        return;
    }

    Rectangle spriteDestination = GetEnemyDestinationRectangle(enemy);
    float barWidth = 122.0f;
    float barHeight = 10.0f;
    float ratio = (float)enemy->health / (float)enemy->maxHealth;

    Rectangle back = {
        spriteDestination.x + spriteDestination.width * 0.5f - barWidth * 0.5f,
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

    DrawRectangleRounded(back, 0.35f, 8, Fade(BLACK, 0.82f));
    DrawRectangleRounded(
        fill,
        0.35f,
        8,
        ratio > 0.50f ? LIME : (ratio > 0.22f ? ORANGE : RED)
    );

    DrawText(
        "ZOMBIE STUDENT",
        (int)(back.x + 5.0f),
        (int)(back.y - 16.0f),
        12,
        RAYWHITE
    );
}

static void DrawZombieEnemy(
    const ZombieEnemy *enemy,
    const ZombieAssets *assets,
    bool showHitboxes
)
{
    if (!enemy->active) return;

    EnemyState renderState = enemy->state;

    
    if (enemy->health <= 0)
    {
        renderState = ENEMY_DYING;
    }

    Texture2D texture = GetEnemyTexture(assets, renderState);
    Rectangle source = GetEnemySourceRectangle(enemy);
    Rectangle destination = GetEnemyDestinationRectangle(enemy);

    DrawEllipse(
        (int)(enemy->body.x + enemy->body.width * 0.5f),
        (int)(enemy->groundY + 2.0f),
        enemy->drawWidth * 0.19f,
        7.0f,
        Fade(BLACK, 0.28f)
    );

    if (enemy->state == ENEMY_ATTACKING && !enemy->damageApplied)
    {
        DrawText(
            "!",
            (int)(enemy->body.x + enemy->body.width * 0.5f - 4.0f),
            (int)(enemy->body.y - 16.0f),
            24,
            RED
        );
    }

    Color tint = WHITE;

    if (enemy->state == ENEMY_HURT && ((int)(enemy->hurtTimer * 28.0f) % 2 == 0))
    {
        tint = (Color){255, 155, 155, 255};
    }

    DrawTexturePro(
        texture,
        source,
        destination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        tint
    );

    DrawEnemyHealthBar(enemy);

    if (showHitboxes)
    {
        DrawRectangleLinesEx(enemy->body, 2.0f, RED);

        if (enemy->state == ENEMY_ATTACKING)
        {
            DrawRectangleLinesEx(enemy->attackBox, 2.0f, ORANGE);
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "IUT Red Box - Level 1 Zombie Student");
    ToggleBorderlessWindowed();

    InitAudioDevice();
    SetTargetFPS(60);

    Texture2D background = LoadTexture("../assets/backgrounds/main_gate.png");
    Texture2D idleTexture = LoadTexture("../assets/player/idle.png");
    Texture2D runTexture = LoadTexture("../assets/player/run.png");
    Texture2D jumpTexture = LoadTexture("../assets/player/jump.png");
    Texture2D swordTexture = LoadTexture("../assets/player/sword.png");
    Texture2D hurtTexture = LoadTexture("../assets/player/hurt.png");
    Texture2D deathTexture = LoadTexture("../assets/player/death.png");
    Texture2D playerVictoryTexture = LoadTexture("../assets/player/victory.png");
    ZombieAssets zombieAssets = LoadZombieAssets();

    Sound swordSwingSound = LoadSound("../assets/audio/sword_swing.wav");
    Sound swordHitSound = LoadSound("../assets/audio/sword_hit.wav");
    Sound playerHitSound = LoadSound("../assets/audio/player_hit.wav");
    Sound enemyTelegraphSound = LoadSound("../assets/audio/enemy_telegraph.wav");
    Sound enemyPushSound = LoadSound("../assets/audio/enemy_push.wav");
    Sound victorySound = LoadSound("../assets/audio/victory.wav");
    Sound defeatSound = LoadSound("../assets/audio/defeat.wav");
    Sound level1IntroSound = LoadSound("../assets/audio/level1_intro.wav");
    Music level1Ambience = LoadMusicStream("../assets/audio/level1_ambience.wav");

    bool swordSwingSoundReady = IsSoundValid(swordSwingSound);
    bool swordHitSoundReady = IsSoundValid(swordHitSound);
    bool playerHitSoundReady = IsSoundValid(playerHitSound);
    bool enemyTelegraphSoundReady = IsSoundValid(enemyTelegraphSound);
    bool enemyPushSoundReady = IsSoundValid(enemyPushSound);
    bool victorySoundReady = IsSoundValid(victorySound);
    bool defeatSoundReady = IsSoundValid(defeatSound);
    bool level1IntroSoundReady = IsSoundValid(level1IntroSound);
    bool level1AmbienceReady = IsMusicValid(level1Ambience);

    if (swordSwingSoundReady) SetSoundVolume(swordSwingSound, 0.42f);
    if (swordHitSoundReady) SetSoundVolume(swordHitSound, 0.62f);
    if (playerHitSoundReady) SetSoundVolume(playerHitSound, 0.48f);
    if (enemyTelegraphSoundReady) SetSoundVolume(enemyTelegraphSound, 0.34f);
    if (enemyPushSoundReady) SetSoundVolume(enemyPushSound, 0.46f);
    if (victorySoundReady) SetSoundVolume(victorySound, 0.50f);
    if (defeatSoundReady) SetSoundVolume(defeatSound, 0.50f);
    if (level1IntroSoundReady) SetSoundVolume(level1IntroSound, 0.52f);

    if (level1AmbienceReady)
    {
        SetMusicVolume(level1Ambience, 0.20f);
        level1Ambience.looping = true;
    }

    bool playerTexturesValid =
        IsTextureValid(background) &&
        IsTextureValid(idleTexture) &&
        IsTextureValid(runTexture) &&
        IsTextureValid(jumpTexture) &&
        IsTextureValid(swordTexture) &&
        IsTextureValid(hurtTexture) &&
        IsTextureValid(deathTexture) &&
        IsTextureValid(playerVictoryTexture);

    if (!playerTexturesValid || !ZombieAssetsAreValid(&zombieAssets))
    {
        TraceLog(LOG_ERROR, "One or more required textures failed to load.");

        if (IsTextureValid(background)) UnloadTexture(background);
        if (IsTextureValid(idleTexture)) UnloadTexture(idleTexture);
        if (IsTextureValid(runTexture)) UnloadTexture(runTexture);
        if (IsTextureValid(jumpTexture)) UnloadTexture(jumpTexture);
        if (IsTextureValid(swordTexture)) UnloadTexture(swordTexture);
        if (IsTextureValid(hurtTexture)) UnloadTexture(hurtTexture);
        if (IsTextureValid(deathTexture)) UnloadTexture(deathTexture);
        if (IsTextureValid(playerVictoryTexture)) UnloadTexture(playerVictoryTexture);
        UnloadZombieAssets(&zombieAssets);

        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    if (
        !PlayerEndStateAssetLayoutIsValid(
            hurtTexture,
            deathTexture,
            playerVictoryTexture
        )
    )
    {
        TraceLog(
            LOG_ERROR,
            "Player hurt/death/victory sheets must use one shared 512x512 equal-cell format."
        );

        UnloadTexture(background);
        UnloadTexture(idleTexture);
        UnloadTexture(runTexture);
        UnloadTexture(jumpTexture);
        UnloadTexture(swordTexture);
        UnloadTexture(hurtTexture);
        UnloadTexture(deathTexture);
        UnloadTexture(playerVictoryTexture);
        UnloadZombieAssets(&zombieAssets);
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    if (!ZombieAssetLayoutIsValid(&zombieAssets))
    {
        TraceLog(
            LOG_ERROR,
            "Zombie sheets must use equal square cells with one shared cell size: idle/walk/attack/death 8 frames, hurt 4 frames, victory 6 frames."
        );

        UnloadTexture(background);
        UnloadTexture(idleTexture);
        UnloadTexture(runTexture);
        UnloadTexture(jumpTexture);
        UnloadTexture(swordTexture);
        UnloadTexture(hurtTexture);
        UnloadTexture(deathTexture);
        UnloadTexture(playerVictoryTexture);
        UnloadZombieAssets(&zombieAssets);
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    SetTextureFilter(hurtTexture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(deathTexture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(playerVictoryTexture, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(zombieAssets.idle, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(zombieAssets.walk, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(zombieAssets.attack, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(zombieAssets.hurt, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(zombieAssets.death, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(zombieAssets.victory, TEXTURE_FILTER_BILINEAR);

    RenderTexture2D gameTarget = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_BILINEAR);

    Rectangle backgroundSource = {
        0.0f,
        0.0f,
        (float)background.width,
        (float)background.height
    };

    Rectangle backgroundDestination = {
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT
    };

    const int idleFrames = 8;
    const int runFrames = 8;
    const int jumpFrames = 6;
    const int swordFrames = 8;
    const int hurtFrames = PLAYER_HURT_FRAMES;
    const int playerDeathFrames = PLAYER_DEATH_FRAMES;
    const int playerVictoryFrames = PLAYER_VICTORY_FRAMES;

    const float idleFrameWidth = (float)idleTexture.width / (float)idleFrames;
    const float runFrameWidth = (float)runTexture.width / (float)runFrames;
    const float jumpFrameWidth = (float)jumpTexture.width / (float)jumpFrames;
    const float swordFrameWidth = (float)swordTexture.width / (float)swordFrames;
    const float hurtFrameWidth = (float)hurtTexture.width / (float)hurtFrames;
    const float playerDeathFrameWidth =
        (float)deathTexture.width / (float)playerDeathFrames;
    const float playerVictoryFrameWidth =
        (float)playerVictoryTexture.width / (float)playerVictoryFrames;

    int currentFrame = 0;
    float frameTimer = 0.0f;

    const float groundY = 530.0f;
    float playerX = 200.0f;
    float playerFeetY = groundY;
    float verticalVelocity = 0.0f;

    const float groundSpeed = 220.0f;
    const float sprintSpeed = 340.0f;
    const float airSpeed = 440.0f;
    const float sprintAirSpeed = 535.0f;
    const float jumpForce = -910.0f;
    const float gravity = 2000.0f;

    int playerHealth = PLAYER_MAX_HEALTH;
    bool isRunning = false;
    bool wasRunning = false;
    bool isSprinting = false;
    bool wasSprinting = false;
    bool isJumping = false;
    bool wasJumping = false;
    bool isSprintJump = false;
    bool isAttacking = false;
    bool facingRight = true;
    bool attackLungeApplied = false;
    bool enemyHitThisAttack = false;
    bool showHitboxes = false;

    bool playerIsHurt = false;
    bool playerIsDying = false;
    bool playerDeathFinished = false;

    float playerHurtTimer = 0.0f;
    float playerDeathTimer = 0.0f;
    float playerDeathFinalPoseTimer = 0.0f;
    float playerVictoryTimer = 0.0f;

    int playerDeathFrame = 0;

    float playerHurtAnchorCenterX =
        playerX + idleFrameWidth * PLAYER_IDLE_SCALE * 0.5f;

    float playerDeathAnchorCenterX =
        playerX + idleFrameWidth * PLAYER_IDLE_SCALE * 0.5f;

    float playerDeathAnchorFeetY = groundY;

    int comboAttackCount = 0;
    float comboResetTimer = 0.0f;
    bool isSpecialAttack = false;

    float playerInvulnerabilityTimer = 0.0f;
    float cameraShakeTimer = 0.0f;
    float impactFlashTimer = 0.0f;

    float playerStandingDrawHeight = (float)idleTexture.height * 0.52f;
    LevelEnemyConfig levelConfig = GetLevelEnemyConfig(CURRENT_LEVEL);

    ZombieEnemy enemy = CreateZombieEnemy(
        760.0f,
        groundY,
        playerStandingDrawHeight,
        (float)zombieAssets.idle.height,
        levelConfig
    );

    GameState gameState = GAME_PLAYING;
    float resultTimer = 0.0f;
    int successfulHits = 0;
    bool victorySoundPlayed = false;
    bool defeatSoundPlayed = false;

    float levelIntroTimer = 0.0f;
    bool ambienceStarted = false;

    if (level1IntroSoundReady)
    {
        PlaySound(level1IntroSound);
    }

    Camera2D camera = {0};
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (level1AmbienceReady)
        {
            UpdateMusicStream(level1Ambience);
        }

        if (gameState == GAME_PLAYING && !ambienceStarted)
        {
            levelIntroTimer += dt;

            if (levelIntroTimer >= 1.35f && level1AmbienceReady)
            {
                PlayMusicStream(level1Ambience);
                ambienceStarted = true;
            }
        }

        float previousPlayerX = playerX;
        isRunning = false;
        isSprinting = false;

        if (playerInvulnerabilityTimer > 0.0f) playerInvulnerabilityTimer -= dt;
        if (impactFlashTimer > 0.0f) impactFlashTimer -= dt;
        if (enemy.attackCooldownTimer > 0.0f) enemy.attackCooldownTimer -= dt;

        if (comboResetTimer > 0.0f && !isAttacking)
        {
            comboResetTimer -= dt;

            if (comboResetTimer <= 0.0f)
            {
                comboAttackCount = 0;
                isSpecialAttack = false;
            }
        }

        bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool moveRight = IsKeyDown(KEY_RIGHT);
        bool moveLeft = IsKeyDown(KEY_LEFT);
        bool movementKeyDown = moveRight || moveLeft;

        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        if (IsKeyPressed(KEY_R))
        {
            playerHealth = PLAYER_MAX_HEALTH;
            playerX = 200.0f;
            playerFeetY = groundY;
            verticalVelocity = 0.0f;
            isJumping = false;
            isAttacking = false;
            playerIsHurt = false;
            playerIsDying = false;
            playerDeathFinished = false;
            playerHurtTimer = 0.0f;
            playerDeathTimer = 0.0f;
            playerDeathFinalPoseTimer = 0.0f;
            playerVictoryTimer = 0.0f;
            playerDeathFrame = 0;

            playerHurtAnchorCenterX =
                playerX + idleFrameWidth * PLAYER_IDLE_SCALE * 0.5f;

            playerDeathAnchorCenterX =
                playerX + idleFrameWidth * PLAYER_IDLE_SCALE * 0.5f;

            playerDeathAnchorFeetY = groundY;

            comboAttackCount = 0;
            comboResetTimer = 0.0f;
            isSpecialAttack = false;
            currentFrame = 0;
            frameTimer = 0.0f;

            enemy = CreateZombieEnemy(
                760.0f,
                groundY,
                playerStandingDrawHeight,
                (float)zombieAssets.idle.height,
                levelConfig
            );

            gameState = GAME_PLAYING;
            resultTimer = 0.0f;
            successfulHits = 0;
            victorySoundPlayed = false;
            defeatSoundPlayed = false;
            levelIntroTimer = 0.0f;
            ambienceStarted = false;

            if (level1AmbienceReady) StopMusicStream(level1Ambience);

            if (level1IntroSoundReady)
            {
                StopSound(level1IntroSound);
                PlaySound(level1IntroSound);
            }
        }

        if (
            gameState == GAME_PLAYING &&
            IsKeyPressed(KEY_SPACE) &&
            !isAttacking &&
            !isJumping &&
            !playerIsHurt &&
            !playerIsDying &&
            playerHealth > 0
        )
        {
            comboAttackCount++;

            if (comboAttackCount >= COMBO_REQUIRED_ATTACKS)
            {
                isSpecialAttack = true;
                comboAttackCount = 0;
            }
            else
            {
                isSpecialAttack = false;
            }

            comboResetTimer = COMBO_RESET_TIME;
            isAttacking = true;
            currentFrame = 0;
            frameTimer = 0.0f;
            attackLungeApplied = false;
            enemyHitThisAttack = false;

            if (swordSwingSoundReady)
            {
                SetSoundPitch(swordSwingSound, isSpecialAttack ? 0.94f : 1.00f);
                PlaySound(swordSwingSound);
            }
        }

        if (
            gameState == GAME_PLAYING &&
            IsKeyPressed(KEY_UP) &&
            !isJumping &&
            !isAttacking &&
            !playerIsHurt &&
            !playerIsDying &&
            playerHealth > 0
        )
        {
            isSprintJump = shiftDown && movementKeyDown;
            verticalVelocity = jumpForce;
            isJumping = true;

            if (moveRight && !moveLeft)
            {
                playerX += isSprintJump ? 48.0f : 32.0f;
            }
            else if (moveLeft && !moveRight)
            {
                playerX -= isSprintJump ? 48.0f : 32.0f;
            }

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        float horizontalSpeed;

        if (isJumping)
        {
            horizontalSpeed = isSprintJump ? sprintAirSpeed : airSpeed;
        }
        else if (shiftDown && movementKeyDown)
        {
            horizontalSpeed = sprintSpeed;
            isSprinting = true;
        }
        else
        {
            horizontalSpeed = groundSpeed;
        }

        if (
            gameState == GAME_PLAYING &&
            !isAttacking &&
            !playerIsHurt &&
            !playerIsDying &&
            playerHealth > 0
        )
        {
            if (moveRight && !moveLeft)
            {
                playerX += horizontalSpeed * dt;
                isRunning = true;
                facingRight = true;
            }
            else if (moveLeft && !moveRight)
            {
                playerX -= horizontalSpeed * dt;
                isRunning = true;
                facingRight = false;
            }
        }

        if (isJumping) isSprinting = false;

        if (isJumping && IsKeyReleased(KEY_UP) && verticalVelocity < -200.0f)
        {
            verticalVelocity *= 0.55f;
        }

        if (isJumping)
        {
            verticalVelocity += gravity * dt;
            playerFeetY += verticalVelocity * dt;

            if (playerFeetY >= groundY && verticalVelocity > 0.0f)
            {
                playerFeetY = groundY;
                verticalVelocity = 0.0f;
                isJumping = false;
                isSprintJump = false;
                currentFrame = 0;
                frameTimer = 0.0f;
            }
        }
        else
        {
            playerFeetY = groundY;
        }

        if (playerIsHurt)
        {
            playerHurtTimer += dt;

            if (playerHurtTimer >= PLAYER_HURT_FRAMES * PLAYER_HURT_FRAME_TIME)
            {
                playerIsHurt = false;
                playerHurtTimer = 0.0f;
                currentFrame = 0;
                frameTimer = 0.0f;
            }
        }

        if (playerIsDying && !playerDeathFinished)
        {
            
            if (playerDeathFrame < PLAYER_DEATH_HOLD_FRAME)
            {
                playerDeathTimer += dt;

                while (
                    playerDeathTimer >= PLAYER_DEATH_FRAME_TIME &&
                    playerDeathFrame < PLAYER_DEATH_HOLD_FRAME
                )
                {
                    playerDeathTimer -= PLAYER_DEATH_FRAME_TIME;
                    playerDeathFrame++;
                }
            }
            else
            {
                
                playerDeathFrame = PLAYER_DEATH_HOLD_FRAME;
                playerDeathFinalPoseTimer += dt;

                if (
                    playerDeathFinalPoseTimer >=
                    PLAYER_DEATH_FINAL_POSE_TIME
                )
                {
                    playerDeathFinished = true;
                }
            }
        }

        if (gameState == GAME_VICTORY)
        {
            playerVictoryTimer += dt;
        }

        Texture2D currentTexture;
        float currentFrameWidth;
        float currentFrameHeight;
        float drawScale;
        float frameDuration;
        int totalFrames;

        if (playerIsDying || playerDeathFinished)
        {
            currentTexture = deathTexture;
            currentFrameWidth = playerDeathFrameWidth;
            currentFrameHeight = (float)deathTexture.height;
            drawScale = PLAYER_DEATH_SCALE;
            totalFrames = playerDeathFrames;
            frameDuration = PLAYER_DEATH_FRAME_TIME;
        }
        else if (gameState == GAME_VICTORY)
        {
            currentTexture = playerVictoryTexture;
            currentFrameWidth = playerVictoryFrameWidth;
            currentFrameHeight = (float)playerVictoryTexture.height;
            drawScale = PLAYER_VICTORY_SCALE;
            totalFrames = playerVictoryFrames;
            frameDuration = PLAYER_VICTORY_FRAME_TIME;
        }
        else if (playerIsHurt)
        {
            currentTexture = hurtTexture;
            currentFrameWidth = hurtFrameWidth;
            currentFrameHeight = (float)hurtTexture.height;
            drawScale = PLAYER_HURT_SCALE;
            totalFrames = hurtFrames;
            frameDuration = PLAYER_HURT_FRAME_TIME;
        }
        else if (isAttacking)
        {
            currentTexture = swordTexture;
            currentFrameWidth = swordFrameWidth;
            currentFrameHeight = (float)swordTexture.height;
            drawScale = PLAYER_ATTACK_SCALE;
            totalFrames = swordFrames;
            frameDuration = 0.08f;
        }
        else if (isJumping)
        {
            currentTexture = jumpTexture;
            currentFrameWidth = jumpFrameWidth;
            currentFrameHeight = (float)jumpTexture.height;
            drawScale = PLAYER_JUMP_SCALE;
            totalFrames = jumpFrames;
            frameDuration = 0.10f;
        }
        else if (isRunning)
        {
            currentTexture = runTexture;
            currentFrameWidth = runFrameWidth;
            currentFrameHeight = (float)runTexture.height;
            drawScale = PLAYER_RUN_SCALE;
            totalFrames = runFrames;
            frameDuration = isSprinting ? 0.075f : 0.12f;
        }
        else
        {
            currentTexture = idleTexture;
            currentFrameWidth = idleFrameWidth;
            currentFrameHeight = (float)idleTexture.height;
            drawScale = PLAYER_IDLE_SCALE;
            totalFrames = idleFrames;
            frameDuration = 0.15f;
        }

        float playerDrawWidth = currentFrameWidth * drawScale;
        float playerDrawHeight = currentFrameHeight * drawScale;
        float visualOffsetY = 0.0f;

        if (isAttacking)
        {
            visualOffsetY = PLAYER_ATTACK_GROUND_OFFSET;
        }
        else if (playerIsHurt)
        {
            visualOffsetY = PLAYER_HURT_GROUND_OFFSET;
        }
        else if (playerIsDying || playerDeathFinished)
        {
            visualOffsetY = PLAYER_DEATH_GROUND_OFFSET;
        }
        else if (gameState == GAME_VICTORY)
        {
            visualOffsetY = PLAYER_VICTORY_GROUND_OFFSET;
        }

        float playerVisualFeetY =
            (playerIsDying || playerDeathFinished)
                ? playerDeathAnchorFeetY
                : playerFeetY;

        float playerDrawY =
            playerVisualFeetY -
            playerDrawHeight +
            visualOffsetY;

        float playerReferenceDrawWidth =
            idleFrameWidth * PLAYER_IDLE_SCALE;

        float playerReferenceCenterX =
            playerX + playerReferenceDrawWidth * 0.5f;

        if (playerIsHurt)
        {
            playerReferenceCenterX = playerHurtAnchorCenterX;
        }
        else if (playerIsDying || playerDeathFinished)
        {
            playerReferenceCenterX = playerDeathAnchorCenterX;
        }

        float playerDrawX =
            playerReferenceCenterX -
            playerDrawWidth * 0.5f;

        if (
            gameState == GAME_PLAYING &&
            !playerIsHurt &&
            !playerIsDying &&
            !playerDeathFinished &&
            !isAttacking &&
            (
                isJumping != wasJumping ||
                (!isJumping && isRunning != wasRunning) ||
                (!isJumping && isSprinting != wasSprinting)
            )
        )
        {
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        wasJumping = isJumping;
        wasRunning = isRunning;
        wasSprinting = isSprinting;

        if (playerIsDying || playerDeathFinished)
        {
            currentFrame = playerDeathFinished
                ? PLAYER_DEATH_HOLD_FRAME
                : playerDeathFrame;
        }
        else if (gameState == GAME_VICTORY)
        {
            currentFrame =
                (int)(playerVictoryTimer / PLAYER_VICTORY_FRAME_TIME) %
                PLAYER_VICTORY_FRAMES;
        }
        else if (playerIsHurt)
        {
            int hurtFrame =
                (int)(playerHurtTimer / PLAYER_HURT_FRAME_TIME);

            if (hurtFrame >= PLAYER_HURT_FRAMES)
            {
                hurtFrame = PLAYER_HURT_FRAMES - 1;
            }

            currentFrame = hurtFrame;
        }
        else if (isAttacking)
        {
            float attackFrameDuration = currentFrame <= 1
                ? 0.09f
                : (currentFrame <= 3
                    ? 0.055f
                    : (currentFrame == 4 ? 0.14f : 0.08f));

            frameTimer += dt;

            if (frameTimer >= attackFrameDuration)
            {
                frameTimer = 0.0f;
                currentFrame++;

                if (currentFrame == 3 && !attackLungeApplied)
                {
                    float lungeDistance = isSpecialAttack ? 42.0f : 24.0f;
                    playerX += facingRight ? lungeDistance : -lungeDistance;
                    attackLungeApplied = true;
                }

                if (currentFrame == 4)
                {
                    cameraShakeTimer = isSpecialAttack ? 0.16f : 0.08f;
                    impactFlashTimer = isSpecialAttack ? 0.10f : 0.05f;
                }

                if (currentFrame >= swordFrames)
                {
                    isAttacking = false;
                    currentFrame = 0;
                    frameTimer = 0.0f;
                    attackLungeApplied = false;
                }
            }
        }
        else if (isJumping)
        {
            if (verticalVelocity < -350.0f) currentFrame = 0;
            else if (verticalVelocity < -120.0f) currentFrame = 1;
            else if (verticalVelocity < 120.0f) currentFrame = 2;
            else if (verticalVelocity < 350.0f) currentFrame = 3;
            else if (verticalVelocity < 550.0f) currentFrame = 4;
            else currentFrame = 5;
        }
        else
        {
            frameTimer += dt;

            if (frameTimer >= frameDuration)
            {
                frameTimer = 0.0f;
                currentFrame = (currentFrame + 1) % totalFrames;
            }
        }

        playerX = ClampFloat(
            playerX,
            0.0f,
            (float)SCREEN_WIDTH - playerReferenceDrawWidth
        );

        float playerReferenceDrawHeight =
            (float)idleTexture.height * PLAYER_IDLE_SCALE;

        Rectangle playerBody = {
            playerX + playerReferenceDrawWidth * 0.30f,
            playerFeetY -
                playerReferenceDrawHeight +
                playerReferenceDrawHeight * 0.18f,
            playerReferenceDrawWidth * 0.40f,
            playerReferenceDrawHeight * 0.77f
        };

        float playerBottomY = playerBody.y + playerBody.height;
        float enemyTopY = enemy.body.y;
        bool playerIsAboveEnemy = playerBottomY < enemyTopY + 50.0f;

        if (
            gameState == GAME_PLAYING &&
            enemy.active &&
            enemy.state != ENEMY_DYING &&
            !playerIsDying &&
            !playerDeathFinished &&
            !playerIsAboveEnemy &&
            CheckCollisionRecs(playerBody, enemy.body)
        )
        {
            float previousBodyX =
                previousPlayerX +
                playerReferenceDrawWidth * 0.30f;

            float previousBodyCenterX =
                previousBodyX +
                playerBody.width * 0.5f;

            float enemyCenterX =
                enemy.body.x +
                enemy.body.width * 0.5f;

            if (previousBodyCenterX <= enemyCenterX)
            {
                playerX =
                    enemy.body.x -
                    playerBody.width -
                    playerReferenceDrawWidth * 0.30f;
            }
            else
            {
                playerX =
                    enemy.body.x +
                    enemy.body.width -
                    playerReferenceDrawWidth * 0.30f;
            }

            playerX = ClampFloat(
                playerX,
                0.0f,
                (float)SCREEN_WIDTH - playerReferenceDrawWidth
            );

            playerBody.x =
                playerX +
                playerReferenceDrawWidth * 0.30f;
        }

        bool playerAttackActive =
            gameState == GAME_PLAYING &&
            isAttacking &&
            currentFrame >= 3 &&
            currentFrame <= 5;

        float playerAttackWidth = isSpecialAttack
            ? PLAYER_SPECIAL_ATTACK_WIDTH
            : PLAYER_NORMAL_ATTACK_WIDTH;

        float playerAttackHeight = isSpecialAttack
            ? PLAYER_SPECIAL_ATTACK_HEIGHT
            : PLAYER_NORMAL_ATTACK_HEIGHT;

        float playerAttackGapLimit = isSpecialAttack
            ? PLAYER_SPECIAL_ATTACK_GAP
            : PLAYER_NORMAL_ATTACK_GAP;

        Rectangle playerAttackBox = {
            0.0f,
            playerBody.y + playerBody.height * 0.13f,
            playerAttackWidth,
            playerAttackHeight
        };

        playerAttackBox.x = facingRight
            ? playerBody.x + playerBody.width * 0.84f
            : playerBody.x - playerAttackBox.width + playerBody.width * 0.16f;

        if (gameState == GAME_PLAYING && enemy.active)
        {
            float playerCenterX = playerBody.x + playerBody.width * 0.5f;
            float enemyCenterX = enemy.body.x + enemy.body.width * 0.5f;
            float distance = fabsf(playerCenterX - enemyCenterX);
            float horizontalGap = 0.0f;

            if (playerBody.x + playerBody.width < enemy.body.x)
            {
                horizontalGap = enemy.body.x - (playerBody.x + playerBody.width);
            }
            else if (enemy.body.x + enemy.body.width < playerBody.x)
            {
                horizontalGap = playerBody.x - (enemy.body.x + enemy.body.width);
            }

            enemy.facingRight = playerCenterX > enemyCenterX;

            if (enemy.state == ENEMY_DYING)
            {
                
                if (enemy.deathFrame < ZOMBIE_DEATH_HOLD_FRAME)
                {
                    enemy.deathTimer += dt;

                    while (
                        enemy.deathTimer >=
                            ZOMBIE_DEATH_FRAME_TIME &&
                        enemy.deathFrame <
                            ZOMBIE_DEATH_HOLD_FRAME
                    )
                    {
                        enemy.deathTimer -=
                            ZOMBIE_DEATH_FRAME_TIME;

                        enemy.deathFrame++;
                    }
                }
                else
                {
                    enemy.deathFrame =
                        ZOMBIE_DEATH_HOLD_FRAME;

                    enemy.deathFinalPoseTimer += dt;

                    if (
                        enemy.deathFinalPoseTimer >=
                        ZOMBIE_DEATH_FINAL_POSE_TIME
                    )
                    {
                        enemy.deathFinished = true;
                    }
                }

       
                enemy.animationFrame =
                    enemy.deathFinished
                        ? ZOMBIE_DEATH_HOLD_FRAME
                        : enemy.deathFrame;
            }
            else if (enemy.state == ENEMY_HURT)
            {
                enemy.hurtTimer += dt;
                int hurtFrame = (int)(enemy.hurtTimer / 0.07f);

                if (hurtFrame >= ZOMBIE_HURT_FRAMES)
                {
                    SetEnemyState(&enemy, ENEMY_IDLE);
                }
                else
                {
                    enemy.animationFrame = hurtFrame;
                }
            }
            else if (enemy.state == ENEMY_ATTACKING)
            {
                enemy.attackTimer += dt;
                enemy.attackBox = GetEnemyAttackBox(&enemy);

                float attackTotalDuration = enemy.attackWindup + 0.34f;
                float progress = ClampFloat(enemy.attackTimer / attackTotalDuration, 0.0f, 0.9999f);
                enemy.animationFrame = (int)(progress * (float)ZOMBIE_ATTACK_FRAMES);

                if (enemy.attackTimer >= enemy.attackWindup && !enemy.damageApplied)
                {
                    if (enemyPushSoundReady) PlaySound(enemyPushSound);

                    if (
                        !playerIsAboveEnemy &&
                        CheckCollisionRecs(enemy.attackBox, playerBody) &&
                        playerInvulnerabilityTimer <= 0.0f &&
                        playerHealth > 0
                    )
                    {
                        playerHealth -= enemy.damage;
                        if (playerHealth < 0) playerHealth = 0;

                        playerInvulnerabilityTimer = 0.80f;
                        cameraShakeTimer = 0.10f;
                        impactFlashTimer = 0.06f;

                        isAttacking = false;
                        isRunning = false;
                        isSprinting = false;
                        isJumping = false;
                        verticalVelocity = 0.0f;
                        playerFeetY = groundY;
                        currentFrame = 0;
                        frameTimer = 0.0f;

                        float currentPlayerVisualCenterX =
                            playerX +
                            playerReferenceDrawWidth * 0.5f;

                        if (playerHealth <= 0)
                        {
                            playerIsHurt = false;
                            playerIsDying = true;
                            playerDeathFinished = false;
                            playerDeathTimer = 0.0f;

                            playerDeathFinalPoseTimer = 0.0f;
                            playerDeathFrame = 0;

                            

                            
                            playerDeathAnchorCenterX =
                                currentPlayerVisualCenterX;

                            playerDeathAnchorFeetY = groundY;
                        }
                        else
                        {
                            playerIsHurt = true;
                            playerHurtTimer = 0.0f;

                           
                            playerHurtAnchorCenterX =
                                currentPlayerVisualCenterX;
                        }

                        if (playerHitSoundReady) PlaySound(playerHitSound);
                    }

                    enemy.damageApplied = true;
                }

                if (enemy.attackTimer >= attackTotalDuration)
                {
                    SetEnemyState(&enemy, ENEMY_IDLE);
                    enemy.attackCooldownTimer = enemy.attackCooldown;
                }
            }
            else
            {
                if (
                    !playerIsAboveEnemy &&
                    horizontalGap <= enemy.attackRange &&
                    enemy.attackCooldownTimer <= 0.0f &&
                    playerHealth > 0
                )
                {
                    SetEnemyState(&enemy, ENEMY_ATTACKING);
                    enemy.attackBox = GetEnemyAttackBox(&enemy);

                    if (enemyTelegraphSoundReady) PlaySound(enemyTelegraphSound);
                }
                else if (
                    distance <= enemy.detectionRange &&
                    horizontalGap > enemy.attackRange &&
                    playerHealth > 0
                )
                {
                    SetEnemyState(&enemy, ENEMY_CHASING);
                    enemy.body.x += (enemy.facingRight ? 1.0f : -1.0f) * enemy.moveSpeed * dt;
                    AdvanceLoopingAnimation(&enemy, ZOMBIE_WALK_FRAMES, 0.105f, dt);
                }
                else
                {
                    SetEnemyState(&enemy, ENEMY_IDLE);
                    AdvanceLoopingAnimation(&enemy, ZOMBIE_IDLE_FRAMES, 0.145f, dt);
                }
            }

            if (
                playerAttackActive &&
                !enemyHitThisAttack &&
                enemy.state != ENEMY_DYING &&
                horizontalGap <= playerAttackGapLimit &&
                CheckCollisionRecs(playerAttackBox, enemy.body)
            )
            {
                int attackDamage = isSpecialAttack
                    ? PLAYER_SPECIAL_DAMAGE
                    : PLAYER_SWORD_DAMAGE;

                DamageEnemy(
                    &enemy,
                    attackDamage,
                    playerCenterX < enemyCenterX
                );

                successfulHits++;
                enemyHitThisAttack = true;
                cameraShakeTimer = 0.10f;
                impactFlashTimer = isSpecialAttack ? 0.10f : 0.06f;

                if (swordHitSoundReady)
                {
                    SetSoundPitch(swordHitSound, isSpecialAttack ? 0.90f : 1.00f);
                    PlaySound(swordHitSound);
                }
            }

            enemy.body.x = ClampFloat(
                enemy.body.x,
                0.0f,
                (float)SCREEN_WIDTH - enemy.body.width
            );
        }

        if (
            gameState == GAME_PLAYING &&
            enemy.active &&
            enemy.state != ENEMY_DYING &&
            !playerIsDying &&
            !playerDeathFinished &&
            !playerIsAboveEnemy &&
            CheckCollisionRecs(playerBody, enemy.body)
        )
        {
            float playerCenterX = playerBody.x + playerBody.width * 0.5f;
            float enemyCenterX = enemy.body.x + enemy.body.width * 0.5f;

            if (enemyCenterX >= playerCenterX)
            {
                enemy.body.x = playerBody.x + playerBody.width;
            }
            else
            {
                enemy.body.x = playerBody.x - enemy.body.width;
            }

            enemy.body.x = ClampFloat(
                enemy.body.x,
                0.0f,
                (float)SCREEN_WIDTH - enemy.body.width
            );
        }

        if (gameState == GAME_PLAYING)
        {
            if (playerDeathFinished)
            {
                gameState = GAME_DEFEAT;
                resultTimer = 0.0f;

                playerDeathFrame = PLAYER_DEATH_HOLD_FRAME;
                playerDeathTimer = 0.0f;
                playerDeathFinalPoseTimer =
                    PLAYER_DEATH_FINAL_POSE_TIME;

                currentFrame = PLAYER_DEATH_HOLD_FRAME;
                frameTimer = 0.0f;

                isAttacking = false;
                isRunning = false;
                isSprinting = false;

                if (enemy.active && enemy.state != ENEMY_DYING)
                {
                    SetEnemyState(&enemy, ENEMY_VICTORY);
                }

                if (level1AmbienceReady) StopMusicStream(level1Ambience);

                if (!defeatSoundPlayed && defeatSoundReady)
                {
                    PlaySound(defeatSound);
                    defeatSoundPlayed = true;
                }
            }
            else if (enemy.deathFinished && playerHealth > 0)
            {
                /*
                   Lock the defeated zombie to the final lying frame before
                   switching into the player-victory presentation.
                */
                enemy.state = ENEMY_DYING;
                enemy.deathFrame = ZOMBIE_DEATH_HOLD_FRAME;
                enemy.animationFrame = ZOMBIE_DEATH_HOLD_FRAME;
                enemy.deathTimer = 0.0f;
                enemy.deathFinalPoseTimer =
                    ZOMBIE_DEATH_FINAL_POSE_TIME;
                enemy.deathFinished = true;

                gameState = GAME_VICTORY;
                resultTimer = 0.0f;
                playerVictoryTimer = 0.0f;
                isAttacking = false;
                isRunning = false;
                isSprinting = false;
                isJumping = false;
                playerFeetY = groundY;

                if (level1AmbienceReady) StopMusicStream(level1Ambience);

                if (!victorySoundPlayed && victorySoundReady)
                {
                    PlaySound(victorySound);
                    victorySoundPlayed = true;
                }
            }
        }
        else
        {
            resultTimer += dt;

            if (gameState == GAME_DEFEAT && enemy.active)
            {
                AdvanceLoopingAnimation(
                    &enemy,
                    ZOMBIE_VICTORY_FRAMES,
                    PLAYER_VICTORY_FRAME_TIME,
                    dt
                );
            }
        }

        if (cameraShakeTimer > 0.0f)
        {
            cameraShakeTimer -= dt;
            camera.offset.x = (float)GetRandomValue(-2, 2);
            camera.offset.y = (float)GetRandomValue(-2, 2);
        }
        else
        {
            camera.offset = (Vector2){0.0f, 0.0f};
        }

        int playerRenderFrame = currentFrame;

        if (playerIsDying || playerDeathFinished)
        {
            playerRenderFrame = playerDeathFinished
                ? PLAYER_DEATH_HOLD_FRAME
                : playerDeathFrame;
        }

        Rectangle sourceRectangle = facingRight
            ? (Rectangle){
                playerRenderFrame * currentFrameWidth,
                0.0f,
                currentFrameWidth,
                currentFrameHeight
            }
            : (Rectangle){
                (playerRenderFrame + 1) * currentFrameWidth,
                0.0f,
                -currentFrameWidth,
                currentFrameHeight
            };

        Rectangle destinationRectangle = {
            playerDrawX,
            playerDrawY,
            playerDrawWidth,
            playerDrawHeight
        };

        BeginTextureMode(gameTarget);
        ClearBackground(BLACK);
        BeginMode2D(camera);

        DrawTexturePro(
            background,
            backgroundSource,
            backgroundDestination,
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        if (isSprinting)
        {
            float dustX = facingRight
                ? playerX + playerDrawWidth * 0.20f
                : playerX + playerDrawWidth * 0.80f;

            DrawCircle(
                (int)dustX,
                (int)(playerFeetY - 4.0f),
                9.0f,
                Fade(LIGHTGRAY, 0.28f)
            );
        }

        if (playerAttackActive)
        {
            float direction = facingRight ? 1.0f : -1.0f;

            Vector2 slashStart = {
                playerX + playerDrawWidth * 0.52f,
                playerDrawY + playerDrawHeight * 0.48f
            };

            if (isSpecialAttack)
            {
                Vector2 upper = {
                    slashStart.x + 145.0f * direction,
                    slashStart.y - 45.0f
                };

                Vector2 middle = {
                    slashStart.x + 165.0f * direction,
                    slashStart.y
                };

                Vector2 lower = {
                    slashStart.x + 145.0f * direction,
                    slashStart.y + 45.0f
                };

                DrawLineEx(slashStart, upper, 12.0f, Fade(GOLD, 0.45f));
                DrawLineEx(slashStart, middle, 10.0f, Fade(ORANGE, 0.70f));
                DrawLineEx(slashStart, lower, 7.0f, Fade(RAYWHITE, 0.85f));
            }
            else
            {
                Vector2 slashEnd = {
                    slashStart.x + 105.0f * direction,
                    slashStart.y
                };

                DrawLineEx(slashStart, slashEnd, 7.0f, Fade(SKYBLUE, 0.50f));
            }
        }

        Color playerTint = WHITE;

        if (
            !playerIsDying &&
            !playerDeathFinished &&
            playerInvulnerabilityTimer > 0.0f &&
            ((int)(playerInvulnerabilityTimer * 18.0f) % 2 == 0)
        )
        {
            playerTint = Fade(WHITE, 0.35f);
        }

        DrawTexturePro(
            currentTexture,
            sourceRectangle,
            destinationRectangle,
            (Vector2){0.0f, 0.0f},
            0.0f,
            playerTint
        );

        DrawZombieEnemy(&enemy, &zombieAssets, showHitboxes);

        if (showHitboxes)
        {
            DrawRectangleLinesEx(playerBody, 2.0f, GREEN);

            if (playerAttackActive)
            {
                DrawRectangleLinesEx(playerAttackBox, 2.0f, SKYBLUE);
            }
        }

        if (IsKeyDown(KEY_F2))
        {
            DrawLine(0, (int)groundY, SCREEN_WIDTH, (int)groundY, YELLOW);
        }

        EndMode2D();

        if (impactFlashTimer > 0.0f)
        {
            DrawRectangle(
                0,
                0,
                SCREEN_WIDTH,
                SCREEN_HEIGHT,
                Fade(SKYBLUE, 0.08f)
            );
        }

        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            112,
            Fade(BLACK, 0.72f)
        );

        
        DrawText(
            "IUT RED BOX",
            24,
            18,
            30,
            RED
        );

        DrawText(
            TextFormat("LEVEL %d", CURRENT_LEVEL),
            25,
            58,
            24,
            RAYWHITE
        );

        DrawText(
            "PROFESSOR",
            690,
            12,
            17,
            RAYWHITE
        );

        Rectangle playerBarBack = {
            690.0f,
            35.0f,
            270.0f,
            20.0f
        };

        Rectangle playerBarFill = {
            playerBarBack.x,
            playerBarBack.y,
            playerBarBack.width *
                (
                    (float)playerHealth /
                    (float)PLAYER_MAX_HEALTH
                ),
            playerBarBack.height
        };

        DrawRectangleRounded(
            playerBarBack,
            0.28f,
            8,
            DARKGRAY
        );

        DrawRectangleRounded(
            playerBarFill,
            0.28f,
            8,
            playerHealth > 30 ? LIME : RED
        );

        DrawRectangleLinesEx(
            playerBarBack,
            2.0f,
            RAYWHITE
        );

        DrawText(
            TextFormat(
                "%d / %d",
                playerHealth,
                PLAYER_MAX_HEALTH
            ),
            786,
            37,
            17,
            BLACK
        );

        const char *comboText =
            isSpecialAttack && isAttacking
                ? "COMBO: SPECIAL ATTACK!"
                : TextFormat(
                    "COMBO: %d / %d",
                    comboAttackCount,
                    COMBO_REQUIRED_ATTACKS
                );

        Rectangle comboBox = {
            690.0f,
            68.0f,
            270.0f,
            29.0f
        };

        Color comboAccent =
            isSpecialAttack && isAttacking
                ? GOLD
                : SKYBLUE;

        DrawRectangleRounded(
            comboBox,
            0.25f,
            8,
            Fade(BLACK, 0.82f)
        );

        DrawRectangleLinesEx(
            comboBox,
            2.0f,
            comboAccent
        );

        int comboTextX =
            isSpecialAttack && isAttacking
                ? 728
                : 772;

        DrawText(
            comboText,
            comboTextX,
            (int)(comboBox.y + 5.0f),
            17,
            comboAccent
        );

        if (isSpecialAttack && isAttacking && playerAttackActive)
        {
            DrawText("THIRD STRIKE!", 410, 145, 25, GOLD);
        }

        if (
            gameState != GAME_PLAYING &&
            resultTimer >= RESULT_PANEL_DELAY
        )
        {
            DrawRectangle(
                0,
                0,
                SCREEN_WIDTH,
                SCREEN_HEIGHT,
                Fade(BLACK, 0.62f)
            );

            Rectangle resultPanel = {230.0f, 185.0f, 540.0f, 250.0f};
            Color accent = gameState == GAME_VICTORY ? LIME : RED;

            DrawRectangleRounded(resultPanel, 0.08f, 12, Fade(BLACK, 0.92f));
            DrawRectangleLinesEx(resultPanel, 3.0f, accent);

            if (gameState == GAME_VICTORY)
            {
                DrawText("LEVEL 1 COMPLETE", 330, 215, 34, LIME);
                DrawText("Zombie Student defeated - area secured", 320, 260, 21, RAYWHITE);
                DrawText(TextFormat("Successful hits: %d", successfulHits), 390, 305, 20, LIGHTGRAY);
                DrawText(TextFormat("Health remaining: %d", playerHealth), 388, 335, 20, LIGHTGRAY);
                DrawText("VICTORY", 435, 375, 26, GOLD);
            }
            else
            {
                DrawText("MISSION FAILED", 357, 215, 34, RED);
                DrawText("The Zombie Student defeated the player", 317, 260, 21, RAYWHITE);
                DrawText(TextFormat("Enemy health remaining: %d", enemy.health), 365, 310, 20, LIGHTGRAY);
                DrawText("ENEMY WINS", 408, 355, 27, ORANGE);
            }

            DrawText("Press R to restart Level 1", 365, 402, 18, GRAY);
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        float realScreenWidth = (float)GetScreenWidth();
        float realScreenHeight = (float)GetScreenHeight();

        float screenScale = fminf(
            realScreenWidth / (float)SCREEN_WIDTH,
            realScreenHeight / (float)SCREEN_HEIGHT
        );

        float scaledWidth = (float)SCREEN_WIDTH * screenScale;
        float scaledHeight = (float)SCREEN_HEIGHT * screenScale;
        float screenOffsetX = (realScreenWidth - scaledWidth) * 0.5f;
        float screenOffsetY = (realScreenHeight - scaledHeight) * 0.5f;

        Rectangle targetSource = {
            0.0f,
            0.0f,
            (float)gameTarget.texture.width,
            -(float)gameTarget.texture.height
        };

        Rectangle targetDestination = {
            screenOffsetX,
            screenOffsetY,
            scaledWidth,
            scaledHeight
        };

        DrawTexturePro(
            gameTarget.texture,
            targetSource,
            targetDestination,
            (Vector2){0.0f, 0.0f},
            0.0f,
            WHITE
        );

        EndDrawing();
    }

    UnloadRenderTexture(gameTarget);

    UnloadTexture(background);
    UnloadTexture(idleTexture);
    UnloadTexture(runTexture);
    UnloadTexture(jumpTexture);
    UnloadTexture(swordTexture);
    UnloadTexture(hurtTexture);
    UnloadTexture(deathTexture);
    UnloadTexture(playerVictoryTexture);
    UnloadZombieAssets(&zombieAssets);

    if (swordSwingSoundReady) UnloadSound(swordSwingSound);
    if (swordHitSoundReady) UnloadSound(swordHitSound);
    if (playerHitSoundReady) UnloadSound(playerHitSound);
    if (enemyTelegraphSoundReady) UnloadSound(enemyTelegraphSound);
    if (enemyPushSoundReady) UnloadSound(enemyPushSound);
    if (victorySoundReady) UnloadSound(victorySound);
    if (defeatSoundReady) UnloadSound(defeatSound);
    if (level1IntroSoundReady) UnloadSound(level1IntroSound);
    if (level1AmbienceReady) UnloadMusicStream(level1Ambience);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}