#include "player.h"
#include "config.h"

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

static void AdvancePlayerLoopingAnimation(
    Player *player,
    float dt
)
{
    player->frameTimer += dt;

    /*
       Keep the unspent time instead of resetting the timer.  This makes
       idle/run playback stable when a frame takes longer than expected.
    */
    while (player->frameTimer >= player->frameDuration)
    {
        player->frameTimer -= player->frameDuration;
        player->currentFrame =
            (player->currentFrame + 1) %
            player->totalFrames;
    }
}

static void AdvancePlayerAttack(
    Player *player,
    float dt
)
{
    player->frameTimer += dt;

    while (player->isAttacking)
    {
        float frameDuration = player->currentFrame <= 1
            ? 0.09f
            : (
                player->currentFrame <= 3
                    ? 0.055f
                    : (
                        player->currentFrame == 4
                            ? 0.14f
                            : 0.08f
                    )
            );

        if (player->frameTimer < frameDuration)
        {
            break;
        }

        player->frameTimer -= frameDuration;
        player->currentFrame++;

        if (
            player->currentFrame == 3 &&
            !player->attackLungeApplied
        )
        {
            float lungeDistance =
                player->specialAttack ? 42.0f : 24.0f;

            player->x += player->facingRight
                ? lungeDistance
                : -lungeDistance;

            player->attackLungeApplied = true;
        }

        if (player->currentFrame == 4)
        {
            player->attackImpactEvent = true;
        }

        if (player->currentFrame >= PLAYER_ATTACK_FRAMES)
        {
            player->isAttacking = false;
            player->currentFrame = 0;
            player->frameTimer = 0.0f;
            player->attackLungeApplied = false;
        }
    }
}

static void SelectPlayerVisualState(
    Player *player,
    const PlayerAssets *assets,
    bool victoryMode
)
{
    if (player->isDying || player->deathFinished)
    {
        player->currentTexture = assets->death;
        player->currentFrameWidth = assets->deathFrameWidth;
        player->currentFrameHeight = (float)assets->death.height;
        player->drawScale = PLAYER_DEATH_SCALE;
        player->totalFrames = PLAYER_DEATH_FRAMES;
        player->frameDuration = PLAYER_DEATH_FRAME_TIME;
    }
    else if (victoryMode)
    {
        player->currentTexture = assets->victory;
        player->currentFrameWidth = assets->victoryFrameWidth;
        player->currentFrameHeight = (float)assets->victory.height;
        player->drawScale = PLAYER_VICTORY_SCALE;
        player->totalFrames = PLAYER_VICTORY_FRAMES;
        player->frameDuration = PLAYER_VICTORY_FRAME_TIME;
    }
    else if (player->isHurt)
    {
        player->currentTexture = assets->hurt;
        player->currentFrameWidth = assets->hurtFrameWidth;
        player->currentFrameHeight = (float)assets->hurt.height;
        player->drawScale = PLAYER_HURT_SCALE;
        player->totalFrames = PLAYER_HURT_FRAMES;
        player->frameDuration = PLAYER_HURT_FRAME_TIME;
    }
    else if (player->isAttacking)
    {
        player->currentTexture = assets->attack;
        player->currentFrameWidth = assets->attackFrameWidth;
        player->currentFrameHeight = (float)assets->attack.height;
        player->drawScale = PLAYER_ATTACK_SCALE;
        player->totalFrames = PLAYER_ATTACK_FRAMES;
        player->frameDuration = 0.08f;
    }
    else if (player->isJumping)
    {
        player->currentTexture = assets->jump;
        player->currentFrameWidth = assets->jumpFrameWidth;
        player->currentFrameHeight = (float)assets->jump.height;
        player->drawScale = PLAYER_JUMP_SCALE;
        player->totalFrames = PLAYER_JUMP_FRAMES;
        player->frameDuration = 0.10f;
    }
    else if (player->isRunning)
    {
        player->currentTexture = assets->run;
        player->currentFrameWidth = assets->runFrameWidth;
        player->currentFrameHeight = (float)assets->run.height;
        player->drawScale = PLAYER_RUN_SCALE;
        player->totalFrames = PLAYER_RUN_FRAMES;
        player->frameDuration = player->isSprinting ? 0.075f : 0.12f;
    }
    else
    {
        player->currentTexture = assets->idle;
        player->currentFrameWidth = assets->idleFrameWidth;
        player->currentFrameHeight = (float)assets->idle.height;
        player->drawScale = PLAYER_IDLE_SCALE;
        player->totalFrames = PLAYER_IDLE_FRAMES;
        player->frameDuration = 0.15f;
    }
}

bool LoadPlayerAssets(PlayerAssets *assets)
{
    if (assets == 0)
    {
        return false;
    }

    *assets = (PlayerAssets){0};

    assets->idle = LoadTexture("../assets/player/idle.png");
    assets->run = LoadTexture("../assets/player/run.png");
    assets->jump = LoadTexture("../assets/player/jump.png");
    assets->attack = LoadTexture("../assets/player/sword.png");
    assets->hurt = LoadTexture("../assets/player/hurt.png");
    assets->death = LoadTexture("../assets/player/death.png");
    assets->victory = LoadTexture("../assets/player/victory.png");

    if (!PlayerAssetsAreValid(assets))
    {
        return false;
    }

    assets->idleFrameWidth =
        (float)assets->idle.width / (float)PLAYER_IDLE_FRAMES;

    assets->runFrameWidth =
        (float)assets->run.width / (float)PLAYER_RUN_FRAMES;

    assets->jumpFrameWidth =
        (float)assets->jump.width / (float)PLAYER_JUMP_FRAMES;

    assets->attackFrameWidth =
        (float)assets->attack.width / (float)PLAYER_ATTACK_FRAMES;

    assets->hurtFrameWidth =
        (float)assets->hurt.width / (float)PLAYER_HURT_FRAMES;

    assets->deathFrameWidth =
        (float)assets->death.width / (float)PLAYER_DEATH_FRAMES;

    assets->victoryFrameWidth =
        (float)assets->victory.width / (float)PLAYER_VICTORY_FRAMES;

    SetTextureFilter(assets->hurt, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(assets->death, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(assets->victory, TEXTURE_FILTER_BILINEAR);

    return true;
}

bool PlayerAssetsAreValid(const PlayerAssets *assets)
{
    if (assets == 0)
    {
        return false;
    }

    return IsTextureValid(assets->idle) &&
           IsTextureValid(assets->run) &&
           IsTextureValid(assets->jump) &&
           IsTextureValid(assets->attack) &&
           IsTextureValid(assets->hurt) &&
           IsTextureValid(assets->death) &&
           IsTextureValid(assets->victory);
}

bool PlayerEndStateAssetLayoutIsValid(const PlayerAssets *assets)
{
    if (assets == 0)
    {
        return false;
    }

    int frameSize = assets->hurt.height;

    if (frameSize <= 0)
    {
        return false;
    }

    return TextureUsesEqualSquareFrames(
               assets->hurt,
               PLAYER_HURT_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->death,
               PLAYER_DEATH_FRAMES,
               frameSize
           ) &&
           TextureUsesEqualSquareFrames(
               assets->victory,
               PLAYER_VICTORY_FRAMES,
               frameSize
           );
}

void UnloadPlayerAssets(PlayerAssets *assets)
{
    if (assets == 0)
    {
        return;
    }

    if (IsTextureValid(assets->idle)) UnloadTexture(assets->idle);
    if (IsTextureValid(assets->run)) UnloadTexture(assets->run);
    if (IsTextureValid(assets->jump)) UnloadTexture(assets->jump);
    if (IsTextureValid(assets->attack)) UnloadTexture(assets->attack);
    if (IsTextureValid(assets->hurt)) UnloadTexture(assets->hurt);
    if (IsTextureValid(assets->death)) UnloadTexture(assets->death);
    if (IsTextureValid(assets->victory)) UnloadTexture(assets->victory);

    *assets = (PlayerAssets){0};
}

void InitPlayer(Player *player, const PlayerAssets *assets, float groundY)
{
    ResetPlayer(player, assets, groundY);
}

void ResetPlayer(Player *player, const PlayerAssets *assets, float groundY)
{
    if (player == 0 || assets == 0)
    {
        return;
    }

    *player = (Player){0};

    player->x = 200.0f;
    player->previousX = player->x;
    player->feetY = groundY;
    player->groundY = groundY;
    player->health = PLAYER_MAX_HEALTH;
    player->facingRight = true;

    player->referenceDrawWidth =
        assets->idleFrameWidth * PLAYER_IDLE_SCALE;

    player->referenceDrawHeight =
        (float)assets->idle.height * PLAYER_IDLE_SCALE;

    player->hurtAnchorCenterX =
        player->x + player->referenceDrawWidth * 0.5f;

    player->deathAnchorCenterX =
        player->x + player->referenceDrawWidth * 0.5f;

    player->deathAnchorFeetY = groundY;

    player->currentTexture = assets->idle;
    player->currentFrameWidth = assets->idleFrameWidth;
    player->currentFrameHeight = (float)assets->idle.height;
    player->drawScale = PLAYER_IDLE_SCALE;
    player->totalFrames = PLAYER_IDLE_FRAMES;
    player->frameDuration = 0.15f;

    RefreshPlayerGeometry(player, assets);
}

void UpdatePlayer(
    Player *player,
    const PlayerAssets *assets,
    float dt,
    bool gameplayEnabled,
    bool victoryMode,
    Sound swordSwingSound,
    bool swordSwingSoundReady
)
{
    if (player == 0 || assets == 0)
    {
        return;
    }

    player->attackImpactEvent = false;
    player->previousX = player->x;
    player->isRunning = false;
    player->isSprinting = false;

    if (player->invulnerabilityTimer > 0.0f)
    {
        player->invulnerabilityTimer -= dt;
    }

    if (player->comboResetTimer > 0.0f && !player->isAttacking)
    {
        player->comboResetTimer -= dt;

        if (player->comboResetTimer <= 0.0f)
        {
            player->comboAttackCount = 0;
            player->specialAttack = false;
        }
    }

    bool shiftDown =
        IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT);

    bool moveRight = IsKeyDown(KEY_RIGHT);
    bool moveLeft = IsKeyDown(KEY_LEFT);
    bool movementKeyDown = moveRight || moveLeft;

    if (
        gameplayEnabled &&
        IsKeyPressed(KEY_SPACE) &&
        !player->isAttacking &&
        !player->isJumping &&
        !player->isHurt &&
        !player->isDying &&
        player->health > 0
    )
    {
        player->comboAttackCount++;

        if (player->comboAttackCount >= COMBO_REQUIRED_ATTACKS)
        {
            player->specialAttack = true;
            player->comboAttackCount = 0;
        }
        else
        {
            player->specialAttack = false;
        }

        player->comboResetTimer = COMBO_RESET_TIME;
        player->isAttacking = true;
        player->currentFrame = 0;
        player->frameTimer = 0.0f;
        player->attackLungeApplied = false;
        player->enemyHitThisAttack = false;

        if (swordSwingSoundReady)
        {
            SetSoundPitch(
                swordSwingSound,
                player->specialAttack ? 0.94f : 1.00f
            );

            PlaySound(swordSwingSound);
        }
    }

    if (
        gameplayEnabled &&
        IsKeyPressed(KEY_UP) &&
        !player->isJumping &&
        !player->isAttacking &&
        !player->isHurt &&
        !player->isDying &&
        player->health > 0
    )
    {
        player->isSprintJump = shiftDown && movementKeyDown;
        player->verticalVelocity = PLAYER_JUMP_FORCE;
        player->isJumping = true;

        if (moveRight && !moveLeft)
        {
            player->x += player->isSprintJump ? 48.0f : 32.0f;
        }
        else if (moveLeft && !moveRight)
        {
            player->x -= player->isSprintJump ? 48.0f : 32.0f;
        }

        player->currentFrame = 0;
        player->frameTimer = 0.0f;
    }

    float horizontalSpeed;

    if (player->isJumping)
    {
        horizontalSpeed = player->isSprintJump
            ? PLAYER_SPRINT_AIR_SPEED
            : PLAYER_AIR_SPEED;
    }
    else if (shiftDown && movementKeyDown)
    {
        horizontalSpeed = PLAYER_SPRINT_SPEED;
        player->isSprinting = true;
    }
    else
    {
        horizontalSpeed = PLAYER_GROUND_SPEED;
    }

    if (
        gameplayEnabled &&
        !player->isAttacking &&
        !player->isHurt &&
        !player->isDying &&
        player->health > 0
    )
    {
        if (moveRight && !moveLeft)
        {
            player->x += horizontalSpeed * dt;
            player->isRunning = true;
            player->facingRight = true;
        }
        else if (moveLeft && !moveRight)
        {
            player->x -= horizontalSpeed * dt;
            player->isRunning = true;
            player->facingRight = false;
        }
    }

    if (player->isJumping)
    {
        player->isSprinting = false;
    }

    if (
        player->isJumping &&
        IsKeyReleased(KEY_UP) &&
        player->verticalVelocity < -200.0f
    )
    {
        player->verticalVelocity *= 0.55f;
    }

    if (player->isJumping)
    {
        player->verticalVelocity += PLAYER_GRAVITY * dt;
        player->feetY += player->verticalVelocity * dt;

        if (
            player->feetY >= player->groundY &&
            player->verticalVelocity > 0.0f
        )
        {
            player->feetY = player->groundY;
            player->verticalVelocity = 0.0f;
            player->isJumping = false;
            player->isSprintJump = false;
            player->currentFrame = 0;
            player->frameTimer = 0.0f;
        }
    }
    else
    {
        player->feetY = player->groundY;
    }

    if (player->isHurt)
    {
        player->hurtTimer += dt;

        if (
            player->hurtTimer >=
            PLAYER_HURT_FRAMES * PLAYER_HURT_FRAME_TIME
        )
        {
            player->isHurt = false;
            player->hurtTimer = 0.0f;
            player->currentFrame = 0;
            player->frameTimer = 0.0f;
        }
    }

    if (player->isDying && !player->deathFinished)
    {
        if (player->deathFrame < PLAYER_DEATH_HOLD_FRAME)
        {
            player->deathTimer += dt;

            while (
                player->deathTimer >= PLAYER_DEATH_FRAME_TIME &&
                player->deathFrame < PLAYER_DEATH_HOLD_FRAME
            )
            {
                player->deathTimer -= PLAYER_DEATH_FRAME_TIME;
                player->deathFrame++;
            }
        }
        else
        {
            player->deathFrame = PLAYER_DEATH_HOLD_FRAME;
            player->deathFinalPoseTimer += dt;

            if (
                player->deathFinalPoseTimer >=
                PLAYER_DEATH_FINAL_POSE_TIME
            )
            {
                player->deathFinished = true;
            }
        }
    }

    if (victoryMode)
    {
        player->victoryTimer += dt;
    }

    SelectPlayerVisualState(player, assets, victoryMode);

    if (
        gameplayEnabled &&
        !player->isHurt &&
        !player->isDying &&
        !player->deathFinished &&
        !player->isAttacking &&
        (
            player->isJumping != player->wasJumping ||
            (
                !player->isJumping &&
                player->isRunning != player->wasRunning
            ) ||
            (
                !player->isJumping &&
                player->isSprinting != player->wasSprinting
            )
        )
    )
    {
        player->currentFrame = 0;
        player->frameTimer = 0.0f;
    }

    player->wasJumping = player->isJumping;
    player->wasRunning = player->isRunning;
    player->wasSprinting = player->isSprinting;

    if (player->isDying || player->deathFinished)
    {
        player->currentFrame = player->deathFinished
            ? PLAYER_DEATH_HOLD_FRAME
            : player->deathFrame;
    }
    else if (victoryMode)
    {
        player->currentFrame =
            (int)(
                player->victoryTimer /
                PLAYER_VICTORY_FRAME_TIME
            ) %
            PLAYER_VICTORY_FRAMES;
    }
    else if (player->isHurt)
    {
        int hurtFrame =
            (int)(player->hurtTimer / PLAYER_HURT_FRAME_TIME);

        if (hurtFrame >= PLAYER_HURT_FRAMES)
        {
            hurtFrame = PLAYER_HURT_FRAMES - 1;
        }

        player->currentFrame = hurtFrame;
    }
    else if (player->isAttacking)
    {
        AdvancePlayerAttack(player, dt);
    }
    else if (player->isJumping)
    {
        if (player->verticalVelocity < -350.0f)
        {
            player->currentFrame = 0;
        }
        else if (player->verticalVelocity < -120.0f)
        {
            player->currentFrame = 1;
        }
        else if (player->verticalVelocity < 120.0f)
        {
            player->currentFrame = 2;
        }
        else if (player->verticalVelocity < 350.0f)
        {
            player->currentFrame = 3;
        }
        else if (player->verticalVelocity < 550.0f)
        {
            player->currentFrame = 4;
        }
        else
        {
            player->currentFrame = 5;
        }
    }
    else
    {
        AdvancePlayerLoopingAnimation(player, dt);
    }

    /*
       An attack can finish in the update above.  Refresh the selected
       texture now so the final attack frame is never drawn as an idle frame.
    */
    SelectPlayerVisualState(player, assets, victoryMode);

    player->referenceDrawWidth =
        assets->idleFrameWidth * PLAYER_IDLE_SCALE;

    player->x = ClampFloat(
        player->x,
        0.0f,
        (float)SCREEN_WIDTH - player->referenceDrawWidth
    );

    RefreshPlayerGeometry(player, assets);

    player->attackActive =
        gameplayEnabled &&
        player->isAttacking &&
        player->currentFrame >= 3 &&
        player->currentFrame <= 5;
}

void RefreshPlayerGeometry(Player *player, const PlayerAssets *assets)
{
    if (player == 0 || assets == 0)
    {
        return;
    }

    player->drawWidth =
        player->currentFrameWidth * player->drawScale;

    player->drawHeight =
        player->currentFrameHeight * player->drawScale;

    player->referenceDrawWidth =
        assets->idleFrameWidth * PLAYER_IDLE_SCALE;

    player->referenceDrawHeight =
        (float)assets->idle.height * PLAYER_IDLE_SCALE;

    float visualOffsetY = 0.0f;

    if (player->isAttacking)
    {
        visualOffsetY = PLAYER_ATTACK_GROUND_OFFSET;
    }
    else if (player->isHurt)
    {
        visualOffsetY = PLAYER_HURT_GROUND_OFFSET;
    }
    else if (player->isDying || player->deathFinished)
    {
        visualOffsetY = PLAYER_DEATH_GROUND_OFFSET;
    }
    else if (player->currentTexture.id == assets->victory.id)
    {
        visualOffsetY = PLAYER_VICTORY_GROUND_OFFSET;
    }

    float visualFeetY =
        (player->isDying || player->deathFinished)
            ? player->deathAnchorFeetY
            : player->feetY;

    player->drawY =
        visualFeetY -
        player->drawHeight +
        visualOffsetY;

    float referenceCenterX =
        player->x +
        player->referenceDrawWidth * 0.5f;

    if (player->isHurt)
    {
        referenceCenterX = player->hurtAnchorCenterX;
    }
    else if (player->isDying || player->deathFinished)
    {
        referenceCenterX = player->deathAnchorCenterX;
    }

    player->drawX =
        referenceCenterX -
        player->drawWidth * 0.5f;

    player->body = (Rectangle){
        player->x + player->referenceDrawWidth * 0.30f,
        player->feetY -
            player->referenceDrawHeight +
            player->referenceDrawHeight * 0.18f,
        player->referenceDrawWidth * 0.40f,
        player->referenceDrawHeight * 0.77f
    };

    float attackWidth = player->specialAttack
        ? PLAYER_SPECIAL_ATTACK_WIDTH
        : PLAYER_NORMAL_ATTACK_WIDTH;

    float attackHeight = player->specialAttack
        ? PLAYER_SPECIAL_ATTACK_HEIGHT
        : PLAYER_NORMAL_ATTACK_HEIGHT;

    player->attackBox = (Rectangle){
        0.0f,
        player->body.y + player->body.height * 0.13f,
        attackWidth,
        attackHeight
    };

    player->attackBox.x = player->facingRight
        ? player->body.x + player->body.width * 0.84f
        : player->body.x -
            player->attackBox.width +
            player->body.width * 0.16f;
}

void ApplyDamageToPlayer(
    Player *player,
    const PlayerAssets *assets,
    int damage
)
{
    if (
        player == 0 ||
        assets == 0 ||
        player->health <= 0 ||
        player->invulnerabilityTimer > 0.0f
    )
    {
        return;
    }

    player->health -= damage;

    if (player->health < 0)
    {
        player->health = 0;
    }

    player->invulnerabilityTimer = 0.80f;

    player->isAttacking = false;
    player->isRunning = false;
    player->isSprinting = false;
    player->isJumping = false;
    player->verticalVelocity = 0.0f;
    player->feetY = player->groundY;
    player->currentFrame = 0;
    player->frameTimer = 0.0f;

    float currentCenterX =
        player->x +
        (
            assets->idleFrameWidth *
            PLAYER_IDLE_SCALE
        ) *
        0.5f;

    if (player->health <= 0)
    {
        player->isHurt = false;
        player->isDying = true;
        player->deathFinished = false;
        player->deathTimer = 0.0f;
        player->deathFinalPoseTimer = 0.0f;
        player->deathFrame = 0;
        player->deathAnchorCenterX = currentCenterX;
        player->deathAnchorFeetY = player->groundY;
    }
    else
    {
        player->isHurt = true;
        player->hurtTimer = 0.0f;
        player->hurtAnchorCenterX = currentCenterX;
    }

    RefreshPlayerGeometry(player, assets);
}

void StartPlayerVictory(Player *player)
{
    if (player == 0)
    {
        return;
    }

    player->victoryTimer = 0.0f;
    player->isAttacking = false;
    player->isRunning = false;
    player->isSprinting = false;
    player->isJumping = false;
    player->feetY = player->groundY;
    player->currentFrame = 0;
    player->frameTimer = 0.0f;
}

void LockPlayerDeathFinalPose(Player *player)
{
    if (player == 0)
    {
        return;
    }

    player->deathFrame = PLAYER_DEATH_HOLD_FRAME;
    player->deathTimer = 0.0f;
    player->deathFinalPoseTimer =
        PLAYER_DEATH_FINAL_POSE_TIME;

    player->deathFinished = true;
    player->currentFrame = PLAYER_DEATH_HOLD_FRAME;
    player->frameTimer = 0.0f;
}

int GetPlayerAttackDamage(const Player *player)
{
    if (player == 0)
    {
        return 0;
    }

    return player->specialAttack
        ? PLAYER_SPECIAL_DAMAGE
        : PLAYER_SWORD_DAMAGE;
}

float GetPlayerAttackGapLimit(const Player *player)
{
    if (player == 0)
    {
        return 0.0f;
    }

    return player->specialAttack
        ? PLAYER_SPECIAL_ATTACK_GAP
        : PLAYER_NORMAL_ATTACK_GAP;
}

void DrawPlayer(const Player *player, bool showHitboxes)
{
    if (player == 0)
    {
        return;
    }

    if (player->isSprinting)
    {
        float dustX = player->facingRight
            ? player->x + player->drawWidth * 0.20f
            : player->x + player->drawWidth * 0.80f;

        DrawCircle(
            (int)dustX,
            (int)(player->feetY - 4.0f),
            9.0f,
            Fade(LIGHTGRAY, 0.28f)
        );
    }

    if (player->attackActive)
    {
        float direction = player->facingRight ? 1.0f : -1.0f;

        Vector2 slashStart = {
            player->x + player->drawWidth * 0.52f,
            player->drawY + player->drawHeight * 0.48f
        };

        if (player->specialAttack)
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

            DrawLineEx(
                slashStart,
                upper,
                12.0f,
                Fade(GOLD, 0.45f)
            );

            DrawLineEx(
                slashStart,
                middle,
                10.0f,
                Fade(ORANGE, 0.70f)
            );

            DrawLineEx(
                slashStart,
                lower,
                7.0f,
                Fade(RAYWHITE, 0.85f)
            );
        }
        else
        {
            Vector2 slashEnd = {
                slashStart.x + 105.0f * direction,
                slashStart.y
            };

            DrawLineEx(
                slashStart,
                slashEnd,
                7.0f,
                Fade(SKYBLUE, 0.50f)
            );
        }
    }

    int renderFrame = player->currentFrame;

    if (player->isDying || player->deathFinished)
    {
        renderFrame = player->deathFinished
            ? PLAYER_DEATH_HOLD_FRAME
            : player->deathFrame;
    }

    Rectangle source;

    
    if (player->isDying || player->deathFinished)
    {
        source = (Rectangle){
            renderFrame * player->currentFrameWidth,
            0.0f,
            player->currentFrameWidth,
            player->currentFrameHeight
        };
    }
    else if (player->facingRight)
    {
        source = (Rectangle){
            renderFrame * player->currentFrameWidth,
            0.0f,
            player->currentFrameWidth,
            player->currentFrameHeight
        };
    }
    else
    {
        source = (Rectangle){
            (renderFrame + 1) * player->currentFrameWidth,
            0.0f,
            -player->currentFrameWidth,
            player->currentFrameHeight
        };
    }

    Rectangle destination = {
        player->drawX,
        player->drawY,
        player->drawWidth,
        player->drawHeight
    };

    Color tint = WHITE;

    if (
        !player->isDying &&
        !player->deathFinished &&
        player->invulnerabilityTimer > 0.0f &&
        ((int)(player->invulnerabilityTimer * 18.0f) % 2 == 0)
    )
    {
        tint = Fade(WHITE, 0.35f);
    }

    DrawTexturePro(
        player->currentTexture,
        source,
        destination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        tint
    );

    if (showHitboxes)
    {
        DrawRectangleLinesEx(player->body, 2.0f, GREEN);

        if (player->attackActive)
        {
            DrawRectangleLinesEx(
                player->attackBox,
                2.0f,
                SKYBLUE
            );
        }
    }
}
