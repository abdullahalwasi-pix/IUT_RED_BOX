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

typedef enum {
    ENEMY_IDLE,
    ENEMY_CHASING,
    ENEMY_ATTACKING,
    ENEMY_HURT,
    ENEMY_DEAD
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
    bool facingRight;
    bool damageApplied;
    bool active;
} DummyEnemy;

static LevelEnemyConfig GetLevelEnemyConfig(int level)
{
    switch (level) {
        case 1: return (LevelEnemyConfig){250,  6,  40.0f, 900.0f, 78.0f, 1.80f, 0.60f };
        case 2: return (LevelEnemyConfig){ 90, 10,  52.0f, 330.0f, 72.0f, 1.45f, 0.50f };
        case 3: return (LevelEnemyConfig){110, 12,  60.0f, 350.0f, 75.0f, 1.30f, 0.46f };
        case 4: return (LevelEnemyConfig){130, 14,  68.0f, 380.0f, 78.0f, 1.18f, 0.42f };
        case 5: return (LevelEnemyConfig){155, 17,  77.0f, 410.0f, 82.0f, 1.05f, 0.38f };
        case 6: return (LevelEnemyConfig){185, 20,  88.0f, 450.0f, 86.0f, 0.92f, 0.34f };
        case 7:
        default:return (LevelEnemyConfig){225, 24, 100.0f, 500.0f, 90.0f, 0.80f, 0.30f };
    }
}

static DummyEnemy CreateDummyEnemy(float x, float groundY, LevelEnemyConfig config)
{
    DummyEnemy enemy = {0};
    enemy.body = (Rectangle){x, groundY - 118.0f, 58.0f, 118.0f};
    enemy.state = ENEMY_IDLE;
    enemy.health = config.maxHealth;
    enemy.maxHealth = config.maxHealth;
    enemy.damage = config.damage;
    enemy.moveSpeed = config.moveSpeed;
    enemy.detectionRange = config.detectionRange;
    enemy.attackRange = config.attackRange;
    enemy.attackCooldown = config.attackCooldown;
    enemy.attackWindup = config.attackWindup;
    enemy.active = true;
    return enemy;
}

static Rectangle GetEnemyAttackBox(const DummyEnemy *enemy)
{
    Rectangle box = {0.0f, enemy->body.y + 44.0f, 72.0f, 50.0f};
    box.x = enemy->facingRight
        ? enemy->body.x + enemy->body.width - 4.0f
        : enemy->body.x - box.width + 4.0f;
    return box;
}

static void DamageEnemy(DummyEnemy *enemy, int damage, bool playerIsLeft)
{
    if (!enemy->active || enemy->state == ENEMY_DEAD) return;

    enemy->health -= damage;
    if (enemy->health <= 0) {
        enemy->health = 0;
        enemy->state = ENEMY_DEAD;
        enemy->deathTimer = 0.0f;
    } else {
        enemy->state = ENEMY_HURT;
        enemy->hurtTimer = 0.24f;
        enemy->body.x += playerIsLeft ? 18.0f : -18.0f;
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "IUT Red Box - Level 1 Dummy Enemy");
    SetTargetFPS(60);

    Texture2D background = LoadTexture("../assets/backgrounds/main_gate.png");
    Texture2D idleTexture = LoadTexture("../assets/player/idle.png");
    Texture2D runTexture = LoadTexture("../assets/player/run.png");
    Texture2D jumpTexture = LoadTexture("../assets/player/jump.png");
    Texture2D swordTexture = LoadTexture("../assets/player/sword.png");

    if (!IsTextureValid(background) || !IsTextureValid(idleTexture) ||
        !IsTextureValid(runTexture) || !IsTextureValid(jumpTexture) ||
        !IsTextureValid(swordTexture)) {
        TraceLog(LOG_ERROR, "One or more required textures failed to load.");
        if (IsTextureValid(background)) UnloadTexture(background);
        if (IsTextureValid(idleTexture)) UnloadTexture(idleTexture);
        if (IsTextureValid(runTexture)) UnloadTexture(runTexture);
        if (IsTextureValid(jumpTexture)) UnloadTexture(jumpTexture);
        if (IsTextureValid(swordTexture)) UnloadTexture(swordTexture);
        CloseWindow();
        return 1;
    }

    Rectangle backgroundSource = {0, 0, (float)background.width, (float)background.height};
    Rectangle backgroundDestination = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    const int idleFrames = 8;
    const int runFrames = 8;
    const int jumpFrames = 6;
    const int swordFrames = 8;

    const float idleFrameWidth = (float)idleTexture.width / idleFrames;
    const float runFrameWidth = (float)runTexture.width / runFrames;
    const float jumpFrameWidth = (float)jumpTexture.width / jumpFrames;
    const float swordFrameWidth = (float)swordTexture.width / swordFrames;

    int currentFrame = 0;
    float frameTimer = 0.0f;

    const float groundY = 530.0f;
    float playerX = 200.0f;
    float playerFeetY = groundY;
    float verticalVelocity = 0.0f;

    const float groundSpeed = 220.0f;
    const float sprintSpeed = 340.0f;
    const float airSpeed = 390.0f;
    const float sprintAirSpeed = 470.0f;
    const float jumpForce = -800.0f;
    const float gravity = 2000.0f;

    int playerHealth = PLAYER_MAX_HEALTH;
    bool isRunning = false, wasRunning = false;
    bool isSprinting = false, wasSprinting = false;
    bool isJumping = false, wasJumping = false;
    bool isSprintJump = false;
    bool isAttacking = false;
    bool facingRight = true;
    bool attackLungeApplied = false;
    bool enemyHitThisAttack = false;
    bool showHitboxes = false;

    /* Three-hit combo system */
    int comboAttackCount = 0;
    float comboResetTimer = 0.0f;
    bool isSpecialAttack = false;

    float playerInvulnerabilityTimer = 0.0f;
    float cameraShakeTimer = 0.0f;
    float impactFlashTimer = 0.0f;

    LevelEnemyConfig levelConfig = GetLevelEnemyConfig(CURRENT_LEVEL);
    DummyEnemy enemy = CreateDummyEnemy(760.0f, groundY, levelConfig);

    GameState gameState = GAME_PLAYING;
    float resultTimer = 0.0f;
    int successfulHits = 0;

    Camera2D camera = {0};
    camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /*
         * Solid collision resolve করার জন্য আগের X position রাখা হয়।
         */
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

        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        if (IsKeyPressed(KEY_R)) {
            playerHealth = PLAYER_MAX_HEALTH;
            playerX = 200.0f;
            playerFeetY = groundY;
            verticalVelocity = 0.0f;
            isJumping = false;
            isAttacking = false;
            comboAttackCount = 0;
            comboResetTimer = 0.0f;
            isSpecialAttack = false;
            currentFrame = 0;
            frameTimer = 0.0f;
            enemy = CreateDummyEnemy(760.0f, groundY, levelConfig);

            gameState = GAME_PLAYING;
            resultTimer = 0.0f;
            successfulHits = 0;
        }

        if (gameState == GAME_PLAYING && IsKeyPressed(KEY_SPACE) && !isAttacking && !isJumping && playerHealth > 0) {
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
        }

        if (gameState == GAME_PLAYING && IsKeyPressed(KEY_UP) && !isJumping && !isAttacking && playerHealth > 0) {
            isSprintJump = shiftDown && movementKeyDown;
            verticalVelocity = jumpForce;
            isJumping = true;

            /*
             * Jump height অপরিবর্তিত রেখে সামান্য forward boost।
             * এতে enemy-এর ওপর দিয়ে পরিষ্কারভাবে পার হওয়া সহজ হবে।
             */
            if (moveRight && !moveLeft)
            {
                playerX += isSprintJump ? 34.0f : 22.0f;
            }
            else if (moveLeft && !moveRight)
            {
                playerX -= isSprintJump ? 34.0f : 22.0f;
            }

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        float horizontalSpeed;
        if (isJumping) horizontalSpeed = isSprintJump ? sprintAirSpeed : airSpeed;
        else if (shiftDown && movementKeyDown) {
            horizontalSpeed = sprintSpeed;
            isSprinting = true;
        } else horizontalSpeed = groundSpeed;

        if (gameState == GAME_PLAYING &&
            !isAttacking &&
            playerHealth > 0) {
            if (moveRight && !moveLeft) {
                playerX += horizontalSpeed * dt;
                isRunning = true;
                facingRight = true;
            } else if (moveLeft && !moveRight) {
                playerX -= horizontalSpeed * dt;
                isRunning = true;
                facingRight = false;
            }
        }

        if (isJumping) isSprinting = false;
        if (isJumping && IsKeyReleased(KEY_UP) && verticalVelocity < -200.0f) verticalVelocity *= 0.55f;

        if (isJumping) {
            verticalVelocity += gravity * dt;
            playerFeetY += verticalVelocity * dt;
            if (playerFeetY >= groundY && verticalVelocity > 0.0f) {
                playerFeetY = groundY;
                verticalVelocity = 0.0f;
                isJumping = false;
                isSprintJump = false;
                currentFrame = 0;
                frameTimer = 0.0f;
            }
        } else playerFeetY = groundY;

        Texture2D currentTexture;
        float currentFrameWidth, currentFrameHeight, drawScale, frameDuration;
        int totalFrames;

        if (isAttacking) {
            currentTexture = swordTexture;
            currentFrameWidth = swordFrameWidth;
            currentFrameHeight = (float)swordTexture.height;
            drawScale = 0.62f;
            totalFrames = swordFrames;
            frameDuration = 0.08f;
        } else if (isJumping) {
            currentTexture = jumpTexture;
            currentFrameWidth = jumpFrameWidth;
            currentFrameHeight = (float)jumpTexture.height;
            drawScale = 0.52f;
            totalFrames = jumpFrames;
            frameDuration = 0.10f;
        } else if (isRunning) {
            currentTexture = runTexture;
            currentFrameWidth = runFrameWidth;
            currentFrameHeight = (float)runTexture.height;
            drawScale = 0.72f;
            totalFrames = runFrames;
            frameDuration = isSprinting ? 0.075f : 0.12f;
        } else {
            currentTexture = idleTexture;
            currentFrameWidth = idleFrameWidth;
            currentFrameHeight = (float)idleTexture.height;
            drawScale = 0.52f;
            totalFrames = idleFrames;
            frameDuration = 0.15f;
        }

        float playerDrawWidth = currentFrameWidth * drawScale;
        float playerDrawHeight = currentFrameHeight * drawScale;
        float visualOffsetY = isAttacking ? 60.0f : 0.0f;
        float playerDrawY = playerFeetY - playerDrawHeight + visualOffsetY;

        if (!isAttacking && (isJumping != wasJumping || (!isJumping && isRunning != wasRunning) ||
            (!isJumping && isSprinting != wasSprinting))) {
            currentFrame = 0;
            frameTimer = 0.0f;
        }
        wasJumping = isJumping;
        wasRunning = isRunning;
        wasSprinting = isSprinting;

        if (isAttacking) {
            float attackFrameDuration = currentFrame <= 1 ? 0.09f :
                currentFrame <= 3 ? 0.055f : currentFrame == 4 ? 0.14f : 0.08f;
            frameTimer += dt;
            if (frameTimer >= attackFrameDuration) {
                frameTimer = 0.0f;
                currentFrame++;
                if (currentFrame == 3 && !attackLungeApplied) {
                    float lungeDistance = isSpecialAttack ? 42.0f : 24.0f;
                    playerX += facingRight ? lungeDistance : -lungeDistance;
                    attackLungeApplied = true;
                }
                if (currentFrame == 4) {
                    cameraShakeTimer = isSpecialAttack ? 0.16f : 0.08f;
                    impactFlashTimer = isSpecialAttack ? 0.10f : 0.05f;
                }
                if (currentFrame >= swordFrames) {
                    isAttacking = false;
                    currentFrame = 0;
                    frameTimer = 0.0f;
                    attackLungeApplied = false;
                }
            }
        } else if (isJumping) {
            if (verticalVelocity < -350.0f) currentFrame = 0;
            else if (verticalVelocity < -120.0f) currentFrame = 1;
            else if (verticalVelocity < 120.0f) currentFrame = 2;
            else if (verticalVelocity < 350.0f) currentFrame = 3;
            else if (verticalVelocity < 550.0f) currentFrame = 4;
            else currentFrame = 5;
        } else {
            frameTimer += dt;
            if (frameTimer >= frameDuration) {
                frameTimer = 0.0f;
                currentFrame = (currentFrame + 1) % totalFrames;
            }
        }

        if (playerX < 0.0f) playerX = 0.0f;
        if (playerX + playerDrawWidth > SCREEN_WIDTH) playerX = SCREEN_WIDTH - playerDrawWidth;

        Rectangle playerBody = {
            playerX + playerDrawWidth * 0.30f,
            playerDrawY + playerDrawHeight * 0.18f,
            playerDrawWidth * 0.40f,
            playerDrawHeight * 0.77f
        };

        /*
         * Player যথেষ্ট ওপরে থাকলে enemy তাকে আঘাত করতে পারবে না।
         * এতে player enemy-এর মাথার ওপর দিয়ে jump করে যেতে পারে।
         */
        float playerBottomY =
            playerBody.y + playerBody.height;

        float enemyTopY =
            enemy.body.y;

        bool playerIsAboveEnemy =
            playerBottomY <
            enemyTopY + 50.0f;

        /* =====================================================
           SOLID PLAYER ↔ ENEMY BODY COLLISION
           ===================================================== */

        /*
         * Player যদি enemy-এর যথেষ্ট ওপরে থাকে, তবে সে মাথার ওপর দিয়ে
         * jump করে পার হতে পারবে। অন্যথায় দুই body একে অপরের ভেতর
         * প্রবেশ করতে পারবে না।
         */
        if (gameState == GAME_PLAYING &&
            enemy.active &&
            enemy.state != ENEMY_DEAD &&
            !playerIsAboveEnemy &&
            CheckCollisionRecs(playerBody, enemy.body))
        {
            float previousBodyX =
                previousPlayerX +
                playerDrawWidth * 0.30f;

            float previousBodyCenterX =
                previousBodyX +
                playerBody.width * 0.50f;

            float enemyCenterX =
                enemy.body.x +
                enemy.body.width * 0.50f;

            if (previousBodyCenterX <= enemyCenterX)
            {
                playerX =
                    enemy.body.x -
                    playerBody.width -
                    playerDrawWidth * 0.30f;
            }
            else
            {
                playerX =
                    enemy.body.x +
                    enemy.body.width -
                    playerDrawWidth * 0.30f;
            }

            if (playerX < 0.0f)
            {
                playerX = 0.0f;
            }

            if (playerX + playerDrawWidth > SCREEN_WIDTH)
            {
                playerX =
                    SCREEN_WIDTH -
                    playerDrawWidth;
            }

            playerBody.x =
                playerX +
                playerDrawWidth * 0.30f;
        }

        bool playerAttackActive =
            gameState == GAME_PLAYING &&
            isAttacking &&
            currentFrame >= 3 &&
            currentFrame <= 5;
        float playerAttackWidth = isSpecialAttack ? 145.0f : 100.0f;
        float playerAttackHeight = isSpecialAttack ? 100.0f : 82.0f;
        Rectangle playerAttackBox = {
            0.0f,
            playerDrawY + playerDrawHeight * 0.30f,
            playerAttackWidth,
            playerAttackHeight
        };
        playerAttackBox.x = facingRight
            ? playerX + playerDrawWidth * 0.58f
            : playerX - playerAttackBox.width * 0.62f;

        if (gameState == GAME_PLAYING &&
            enemy.active) {
            float playerCenterX = playerBody.x + playerBody.width * 0.5f;
            float enemyCenterX = enemy.body.x + enemy.body.width * 0.5f;
            float distance = fabsf(playerCenterX - enemyCenterX);
            enemy.facingRight = playerCenterX > enemyCenterX;

            if (enemy.state == ENEMY_DEAD) {
                enemy.deathTimer += dt;
                if (enemy.deathTimer >= 1.0f) enemy.active = false;
            } else if (enemy.state == ENEMY_HURT) {
                enemy.hurtTimer -= dt;
                if (enemy.hurtTimer <= 0.0f) enemy.state = ENEMY_IDLE;
            } else if (enemy.state == ENEMY_ATTACKING) {
                enemy.attackTimer += dt;
                enemy.attackBox = GetEnemyAttackBox(&enemy);
                if (enemy.attackTimer >= enemy.attackWindup &&
                    !enemy.damageApplied) {
                    if (!playerIsAboveEnemy &&
                        CheckCollisionRecs(enemy.attackBox, playerBody) &&
                        playerInvulnerabilityTimer <= 0.0f &&
                        playerHealth > 0) {
                        playerHealth -= enemy.damage;
                        if (playerHealth < 0) playerHealth = 0;
                        playerInvulnerabilityTimer = 0.80f;
                        cameraShakeTimer = 0.10f;
                    }
                    enemy.damageApplied = true;
                }
                if (enemy.attackTimer >= enemy.attackWindup + 0.32f) {
                    enemy.state = ENEMY_IDLE;
                    enemy.attackTimer = 0.0f;
                    enemy.damageApplied = false;
                    enemy.attackCooldownTimer = enemy.attackCooldown;
                }
            } else {
                if (!playerIsAboveEnemy &&
                    distance <= enemy.attackRange &&
                    enemy.attackCooldownTimer <= 0.0f &&
                    playerHealth > 0) {
                    enemy.state = ENEMY_ATTACKING;
                    enemy.attackTimer = 0.0f;
                    enemy.damageApplied = false;
                } else if (distance <= enemy.detectionRange && distance > enemy.attackRange && playerHealth > 0) {
                    enemy.state = ENEMY_CHASING;
                    enemy.body.x += (enemy.facingRight ? 1.0f : -1.0f) * enemy.moveSpeed * dt;
                } else enemy.state = ENEMY_IDLE;
            }

            if (playerAttackActive && !enemyHitThisAttack && enemy.state != ENEMY_DEAD &&
                CheckCollisionRecs(playerAttackBox, enemy.body)) {
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
            }

            if (enemy.body.x < 0.0f) enemy.body.x = 0.0f;
            if (enemy.body.x + enemy.body.width > SCREEN_WIDTH) enemy.body.x = SCREEN_WIDTH - enemy.body.width;
        }

        /* -------------------- ENEMY MOVEMENT COLLISION FIX -------------------- */

        /*
         * Enemy chase করে player-এর body-এর মধ্যে ঢুকে গেলে enemy-কে
         * player-এর বাইরের edge-এ থামিয়ে দেওয়া হয়।
         */
        if (gameState == GAME_PLAYING &&
            enemy.active &&
            enemy.state != ENEMY_DEAD &&
            !playerIsAboveEnemy &&
            CheckCollisionRecs(playerBody, enemy.body))
        {
            float playerCenterX =
                playerBody.x +
                playerBody.width * 0.50f;

            float enemyCenterX =
                enemy.body.x +
                enemy.body.width * 0.50f;

            if (enemyCenterX >= playerCenterX)
            {
                enemy.body.x =
                    playerBody.x +
                    playerBody.width;
            }
            else
            {
                enemy.body.x =
                    playerBody.x -
                    enemy.body.width;
            }

            if (enemy.body.x < 0.0f)
            {
                enemy.body.x = 0.0f;
            }

            if (enemy.body.x + enemy.body.width > SCREEN_WIDTH)
            {
                enemy.body.x =
                    SCREEN_WIDTH -
                    enemy.body.width;
            }
        }

        /* -------------------- MATCH RESULT -------------------- */

        if (gameState == GAME_PLAYING)
        {
            if (playerHealth <= 0)
            {
                gameState = GAME_DEFEAT;
                resultTimer = 0.0f;

                isAttacking = false;
                isRunning = false;
                isSprinting = false;

                if (enemy.active &&
                    enemy.state != ENEMY_DEAD)
                {
                    enemy.state = ENEMY_IDLE;
                }
            }
            else if (!enemy.active)
            {
                gameState = GAME_VICTORY;
                resultTimer = 0.0f;

                isAttacking = false;
                isRunning = false;
                isSprinting = false;
            }
        }
        else
        {
            resultTimer += dt;
        }

        if (cameraShakeTimer > 0.0f) {
            cameraShakeTimer -= dt;
            camera.offset.x = (float)GetRandomValue(-2, 2);
            camera.offset.y = (float)GetRandomValue(-2, 2);
        } else camera.offset = (Vector2){0.0f, 0.0f};

        Rectangle sourceRectangle = facingRight
            ? (Rectangle){currentFrame * currentFrameWidth, 0.0f, currentFrameWidth, currentFrameHeight}
            : (Rectangle){(currentFrame + 1) * currentFrameWidth, 0.0f, -currentFrameWidth, currentFrameHeight};

        Rectangle destinationRectangle = {
            playerX,
            playerDrawY,
            playerDrawWidth,
            playerDrawHeight
        };

        /*
         * Victory হলে player আনন্দে হালকা bounce করবে।
         */
        if (gameState == GAME_VICTORY)
        {
            destinationRectangle.y -=
                fabsf(sinf(resultTimer * 4.2f)) *
                9.0f;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        DrawTexturePro(background, backgroundSource, backgroundDestination, (Vector2){0}, 0.0f, WHITE);

        if (enemy.active) {
            Color enemyColor = MAROON;
            if (enemy.state == ENEMY_CHASING) enemyColor = RED;
            else if (enemy.state == ENEMY_ATTACKING) enemyColor = ORANGE;
            else if (enemy.state == ENEMY_HURT) enemyColor = YELLOW;
            else if (enemy.state == ENEMY_DEAD) enemyColor = Fade(DARKGRAY, 0.70f);

            DrawRectangleRounded(enemy.body, 0.16f, 8, enemyColor);
            DrawRectangleLinesEx(enemy.body, 3.0f, BLACK);
            DrawCircle((int)(enemy.body.x + 17.0f), (int)(enemy.body.y + 27.0f), 5.0f, BLACK);
            DrawCircle((int)(enemy.body.x + 41.0f), (int)(enemy.body.y + 27.0f), 5.0f, BLACK);
            const char *enemyFaceText =
                gameState == GAME_DEFEAT
                ? "WIN"
                : (enemy.state == ENEMY_ATTACKING ? "!" : "Z");

            DrawText(
                enemyFaceText,
                (int)(enemy.body.x +
                    (gameState == GAME_DEFEAT ? 7.0f : 22.0f)),
                (int)(enemy.body.y + 48.0f),
                gameState == GAME_DEFEAT ? 16 : 22,
                BLACK
            );

            float healthRatio = (float)enemy.health / (float)enemy.maxHealth;
            Rectangle barBack = {enemy.body.x - 4.0f, enemy.body.y - 15.0f, enemy.body.width + 8.0f, 8.0f};
            Rectangle barFill = {barBack.x, barBack.y, barBack.width * healthRatio, barBack.height};
            DrawRectangleRec(barBack, Fade(BLACK, 0.75f));
            DrawRectangleRec(barFill, LIME);

            if (enemy.state == ENEMY_ATTACKING)
            {
                /*
                 * Attack hitbox logic active থাকবে, কিন্তু gameplay-তে
                 * কোনো square/rectangle দেখা যাবে না।
                 * শুধু enemy-এর "!" warning থাকবে।
                 */
                enemy.attackBox =
                    GetEnemyAttackBox(&enemy);
            }
        }

        if (isSprinting) {
            float dustX = facingRight ? playerX + playerDrawWidth * 0.20f : playerX + playerDrawWidth * 0.80f;
            DrawCircle((int)dustX, (int)(playerFeetY - 4.0f), 9.0f, Fade(LIGHTGRAY, 0.28f));
        }

        if (playerAttackActive) {
            float dir = facingRight ? 1.0f : -1.0f;
            Vector2 slashStart = {
                playerX + playerDrawWidth * 0.52f,
                playerDrawY + playerDrawHeight * 0.48f
            };

            if (isSpecialAttack)
            {
                Vector2 upper = {
                    slashStart.x + 145.0f * dir,
                    slashStart.y - 45.0f
                };
                Vector2 middle = {
                    slashStart.x + 165.0f * dir,
                    slashStart.y
                };
                Vector2 lower = {
                    slashStart.x + 145.0f * dir,
                    slashStart.y + 45.0f
                };

                DrawLineEx(slashStart, upper, 12.0f, Fade(GOLD, 0.45f));
                DrawLineEx(slashStart, middle, 10.0f, Fade(ORANGE, 0.70f));
                DrawLineEx(slashStart, lower, 7.0f, Fade(RAYWHITE, 0.85f));
            }
            else
            {
                Vector2 slashEnd = {
                    slashStart.x + 105.0f * dir,
                    slashStart.y
                };
                DrawLineEx(slashStart, slashEnd, 7.0f, Fade(SKYBLUE, 0.50f));
            }
        }

        Color playerTint = WHITE;
        if (playerInvulnerabilityTimer > 0.0f && ((int)(playerInvulnerabilityTimer * 18.0f) % 2 == 0))
            playerTint = Fade(WHITE, 0.35f);

        DrawTexturePro(
            currentTexture,
            sourceRectangle,
            destinationRectangle,
            (Vector2){0},
            0.0f,
            playerTint
        );

        /*
         * Victory celebration:
         * Player sprite শুধু হালকা up-down bounce করবে।
         * কোনো procedural হাত, circle, spark বা extra drawing নেই।
         */

        if (showHitboxes) {
            DrawRectangleLinesEx(playerBody, 2.0f, GREEN);
            if (playerAttackActive) DrawRectangleLinesEx(playerAttackBox, 2.0f, SKYBLUE);
            if (enemy.active) {
                DrawRectangleLinesEx(enemy.body, 2.0f, RED);
                if (enemy.state == ENEMY_ATTACKING) DrawRectangleLinesEx(enemy.attackBox, 2.0f, ORANGE);
            }
        }

        if (IsKeyDown(KEY_F2)) DrawLine(0, (int)groundY, SCREEN_WIDTH, (int)groundY, YELLOW);
        EndMode2D();

        if (impactFlashTimer > 0.0f)
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(SKYBLUE, 0.08f));

        DrawRectangle(0, 0, SCREEN_WIDTH, 118, Fade(BLACK, 0.68f));
        DrawText(TextFormat("IUT RED BOX  |  LEVEL %d", CURRENT_LEVEL), 24, 16, 28, WHITE);
        DrawText("LEFT/RIGHT Move | SHIFT Sprint | UP Jump | SPACE Attack", 24, 52, 18, LIGHTGRAY);
        DrawText("F1 Debug hitboxes | F2 Ground line | R Restart", 24, 80, 17, GRAY);

        const char *comboText = isSpecialAttack && isAttacking
            ? "COMBO: SPECIAL ATTACK!"
            : TextFormat("COMBO: %d / %d", comboAttackCount, COMBO_REQUIRED_ATTACKS);

        DrawText(
            comboText,
            420,
            82,
            18,
            isSpecialAttack && isAttacking ? GOLD : LIGHTGRAY
        );

        DrawText("PLAYER", 690, 18, 17, WHITE);
        Rectangle playerBarBack = {690.0f, 43.0f, 270.0f, 20.0f};
        Rectangle playerBarFill = {690.0f, 43.0f, 270.0f * ((float)playerHealth / PLAYER_MAX_HEALTH), 20.0f};
        DrawRectangleRec(playerBarBack, DARKGRAY);
        DrawRectangleRec(playerBarFill, playerHealth > 30 ? LIME : RED);
        DrawRectangleLinesEx(playerBarBack, 2.0f, WHITE);
        DrawText(TextFormat("%d / %d", playerHealth, PLAYER_MAX_HEALTH), 786, 44, 17, BLACK);

        if (isSpecialAttack && isAttacking && playerAttackActive)
        {
            DrawText(
                "THIRD STRIKE!",
                410,
                145,
                25,
                GOLD
            );
        }

        /* -------------------- PROFESSIONAL RESULT SCREEN -------------------- */

        if (gameState != GAME_PLAYING)
        {
            DrawRectangle(
                0,
                0,
                SCREEN_WIDTH,
                SCREEN_HEIGHT,
                Fade(BLACK, 0.62f)
            );

            Rectangle resultPanel = {
                230.0f,
                185.0f,
                540.0f,
                250.0f
            };

            Color accent =
                gameState == GAME_VICTORY
                ? LIME
                : RED;

            DrawRectangleRounded(
                resultPanel,
                0.08f,
                12,
                Fade(BLACK, 0.92f)
            );

            DrawRectangleLinesEx(
                resultPanel,
                3.0f,
                accent
            );

            if (gameState == GAME_VICTORY)
            {
                DrawText(
                    "LEVEL 1 COMPLETE",
                    330,
                    215,
                    34,
                    LIME
                );

                DrawText(
                    "Enemy defeated - area secured",
                    357,
                    260,
                    21,
                    RAYWHITE
                );

                DrawText(
                    TextFormat(
                        "Successful hits: %d",
                        successfulHits
                    ),
                    390,
                    305,
                    20,
                    LIGHTGRAY
                );

                DrawText(
                    TextFormat(
                        "Health remaining: %d",
                        playerHealth
                    ),
                    388,
                    335,
                    20,
                    LIGHTGRAY
                );

                DrawText(
                    "VICTORY",
                    435,
                    375,
                    26,
                    GOLD
                );
            }
            else
            {
                DrawText(
                    "MISSION FAILED",
                    357,
                    215,
                    34,
                    RED
                );

                DrawText(
                    "The enemy has defeated the player",
                    330,
                    260,
                    21,
                    RAYWHITE
                );

                DrawText(
                    TextFormat(
                        "Enemy health remaining: %d",
                        enemy.health
                    ),
                    365,
                    310,
                    20,
                    LIGHTGRAY
                );

                DrawText(
                    "ENEMY WINS",
                    408,
                    355,
                    27,
                    ORANGE
                );
            }

            DrawText(
                "Press R to restart Level 1",
                365,
                402,
                18,
                GRAY
            );
        }

        EndDrawing();
    }

    UnloadTexture(background);
    UnloadTexture(idleTexture);
    UnloadTexture(runTexture);
    UnloadTexture(jumpTexture);
    UnloadTexture(swordTexture);
    CloseWindow();
    return 0;
}