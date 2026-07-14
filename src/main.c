#include "raylib.h"

int main(void)
{
    const int screenWidth = 1000;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "IUT Red Box");
    SetTargetFPS(60);

    /* Background texture */
    Texture2D mainGateBackground =
        LoadTexture("../assets/backgrounds/main_gate.png");

    /* Player textures */
    Texture2D idleTexture =
        LoadTexture("../assets/player/idle.png");

    Texture2D runTexture =
        LoadTexture("../assets/player/run.png");

    Texture2D jumpTexture =
        LoadTexture("../assets/player/jump.png");

    Texture2D swordTexture =
        LoadTexture("../assets/player/sword.png");

    /* Texture validation */
    if (!IsTextureValid(mainGateBackground) ||
        !IsTextureValid(idleTexture) ||
        !IsTextureValid(runTexture) ||
        !IsTextureValid(jumpTexture) ||
        !IsTextureValid(swordTexture))
    {
        TraceLog(LOG_ERROR, "One or more textures failed to load.");

        if (IsTextureValid(mainGateBackground))
        {
            UnloadTexture(mainGateBackground);
        }

        if (IsTextureValid(idleTexture))
        {
            UnloadTexture(idleTexture);
        }

        if (IsTextureValid(runTexture))
        {
            UnloadTexture(runTexture);
        }

        if (IsTextureValid(jumpTexture))
        {
            UnloadTexture(jumpTexture);
        }

        if (IsTextureValid(swordTexture))
        {
            UnloadTexture(swordTexture);
        }

        CloseWindow();
        return 1;
    }

    /* Background rectangles */
    Rectangle backgroundSource = {
        0.0f,
        0.0f,
        (float)mainGateBackground.width,
        (float)mainGateBackground.height
    };

    Rectangle backgroundDestination = {
        0.0f,
        0.0f,
        (float)screenWidth,
        (float)screenHeight
    };

    Vector2 backgroundOrigin = {
        0.0f,
        0.0f
    };

    /* Animation frame counts */
    const int idleFrames = 8;
    const int runFrames = 8;
    const int jumpFrames = 6;
    const int swordFrames = 8;

    /* Frame dimensions */
    float idleFrameWidth =
        (float)idleTexture.width / idleFrames;

    float idleFrameHeight =
        (float)idleTexture.height;

    float runFrameWidth =
        (float)runTexture.width / runFrames;

    float runFrameHeight =
        (float)runTexture.height;

    float jumpFrameWidth =
        (float)jumpTexture.width / jumpFrames;

    float jumpFrameHeight =
        (float)jumpTexture.height;

    float swordFrameWidth =
        (float)swordTexture.width / swordFrames;

    float swordFrameHeight =
        (float)swordTexture.height;

    /* Animation state */
    int currentFrame = 0;
    float frameTimer = 0.0f;

    /* Player position */
    Vector2 playerPosition = {
        350.0f,
        100.0f
    };

    /* Movement speeds */
    float groundSpeed = 220.0f;
    float sprintSpeed = 340.0f;

    float airSpeed = 330.0f;
    float sprintAirSpeed = 390.0f;

    /* Jump physics */
    float verticalVelocity = 0.0f;
    float jumpForce = -800.0f;
    float gravity = 2000.0f;

    /*
     * এই মানটি background-এর road অনুযায়ী
     * পরে সামান্য পরিবর্তন করা যাবে।
     */
    float groundY = 530.0f;

    /* Player states */
    bool isRunning = false;
    bool wasRunning = false;

    bool isSprinting = false;
    bool wasSprinting = false;

    bool isJumping = false;
    bool wasJumping = false;

    bool isSprintJump = false;
    bool isAttacking = false;
    bool facingRight = true;

    /* Attack effects */
    bool attackLungeApplied = false;

    float cameraShakeTimer = 0.0f;
    float impactFlashTimer = 0.0f;

    bool showAttackHitbox = false;

    /* Camera */
    Camera2D camera = {0};

    camera.target = (Vector2){0.0f, 0.0f};
    camera.offset = (Vector2){0.0f, 0.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        isRunning = false;
        isSprinting = false;

        /* Input states */
        bool shiftDown =
            IsKeyDown(KEY_LEFT_SHIFT) ||
            IsKeyDown(KEY_RIGHT_SHIFT);

        bool moveRight =
            IsKeyDown(KEY_RIGHT);

        bool moveLeft =
            IsKeyDown(KEY_LEFT);

        bool movementKeyDown =
            moveRight || moveLeft;

        /* Debug hitbox toggle */
        if (IsKeyPressed(KEY_F1))
        {
            showAttackHitbox = !showAttackHitbox;
        }

        /* Ground attack */
        if (IsKeyPressed(KEY_SPACE) &&
            !isAttacking &&
            !isJumping)
        {
            isAttacking = true;

            currentFrame = 0;
            frameTimer = 0.0f;

            attackLungeApplied = false;
        }

        /* Jump */
        if (IsKeyPressed(KEY_UP) &&
            !isJumping &&
            !isAttacking)
        {
            isSprintJump =
                shiftDown &&
                movementKeyDown;

            verticalVelocity = jumpForce;
            isJumping = true;

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        /* Select movement speed */
        float horizontalSpeed;

        if (isJumping)
        {
            horizontalSpeed =
                isSprintJump ?
                sprintAirSpeed :
                airSpeed;
        }
        else
        {
            if (shiftDown && movementKeyDown)
            {
                horizontalSpeed = sprintSpeed;
                isSprinting = true;
            }
            else
            {
                horizontalSpeed = groundSpeed;
            }
        }

        /* Movement is locked while attacking */
        if (!isAttacking)
        {
            if (moveRight)
            {
                playerPosition.x +=
                    horizontalSpeed * deltaTime;

                isRunning = true;
                facingRight = true;
            }

            if (moveLeft)
            {
                playerPosition.x -=
                    horizontalSpeed * deltaTime;

                isRunning = true;
                facingRight = false;
            }
        }

        if (isJumping)
        {
            isSprinting = false;
        }

        /* Variable jump height */
        if (isJumping &&
            IsKeyReleased(KEY_UP) &&
            verticalVelocity < -200.0f)
        {
            verticalVelocity *= 0.55f;
        }

        /* Gravity */
        if (isJumping)
        {
            verticalVelocity +=
                gravity * deltaTime;

            playerPosition.y +=
                verticalVelocity * deltaTime;
        }

        /* Camera shake */
        if (cameraShakeTimer > 0.0f)
        {
            cameraShakeTimer -= deltaTime;

            camera.offset.x =
                (float)GetRandomValue(-2, 2);

            camera.offset.y =
                (float)GetRandomValue(-2, 2);
        }
        else
        {
            camera.offset = (Vector2){
                0.0f,
                0.0f
            };
        }

        /* Impact flash */
        if (impactFlashTimer > 0.0f)
        {
            impactFlashTimer -= deltaTime;
        }

        /* Current animation information */
        Texture2D currentTexture;

        float currentFrameWidth;
        float currentFrameHeight;
        float drawScale;
        float frameDuration;

        int totalFrames;

        /*
         * Priority:
         * Attack → Jump → Run/Sprint → Idle
         */
        if (isAttacking)
        {
            currentTexture = swordTexture;

            currentFrameWidth = swordFrameWidth;
            currentFrameHeight = swordFrameHeight;

            drawScale = 0.62f;

            totalFrames = swordFrames;
            frameDuration = 0.08f;
        }
        else if (isJumping)
        {
            currentTexture = jumpTexture;

            currentFrameWidth = jumpFrameWidth;
            currentFrameHeight = jumpFrameHeight;

            drawScale = 0.52f;

            totalFrames = jumpFrames;
            frameDuration = 0.10f;
        }
        else if (isRunning)
        {
            currentTexture = runTexture;

            currentFrameWidth = runFrameWidth;
            currentFrameHeight = runFrameHeight;

            drawScale = 0.72f;

            totalFrames = runFrames;

            frameDuration =
                isSprinting ?
                0.075f :
                0.12f;
        }
        else
        {
            currentTexture = idleTexture;

            currentFrameWidth = idleFrameWidth;
            currentFrameHeight = idleFrameHeight;

            drawScale = 0.52f;

            totalFrames = idleFrames;
            frameDuration = 0.15f;
        }

        float playerDrawWidth =
            currentFrameWidth * drawScale;

        float playerDrawHeight =
            currentFrameHeight * drawScale;

        float playerGroundPosition =
            groundY - playerDrawHeight;

        /* Ground alignment */
        if (!isJumping)
        {
            playerPosition.y =
                playerGroundPosition;
        }

        /* Landing */
        if (isJumping &&
            playerPosition.y >= playerGroundPosition &&
            verticalVelocity > 0.0f)
        {
            playerPosition.y =
                playerGroundPosition;

            verticalVelocity = 0.0f;

            isJumping = false;
            isSprintJump = false;

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        /* Animation state change */
        if (!isAttacking &&
            (isJumping != wasJumping ||
             (!isJumping &&
              isRunning != wasRunning) ||
             (!isJumping &&
              isSprinting != wasSprinting)))
        {
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        wasJumping = isJumping;
        wasRunning = isRunning;
        wasSprinting = isSprinting;

        /* Sword attack timing */
        if (isAttacking)
        {
            float currentAttackFrameDuration;

            if (currentFrame <= 1)
            {
                currentAttackFrameDuration =
                    0.09f;
            }
            else if (currentFrame <= 3)
            {
                currentAttackFrameDuration =
                    0.055f;
            }
            else if (currentFrame == 4)
            {
                currentAttackFrameDuration =
                    0.14f;
            }
            else
            {
                currentAttackFrameDuration =
                    0.08f;
            }

            frameTimer += deltaTime;

            if (frameTimer >=
                currentAttackFrameDuration)
            {
                frameTimer = 0.0f;
                currentFrame++;

                /* Attack lunge */
                if (currentFrame == 3 &&
                    !attackLungeApplied)
                {
                    float lungeDistance =
                        24.0f;

                    if (facingRight)
                    {
                        playerPosition.x +=
                            lungeDistance;
                    }
                    else
                    {
                        playerPosition.x -=
                            lungeDistance;
                    }

                    attackLungeApplied = true;
                }

                /* Impact effect */
                if (currentFrame == 4)
                {
                    cameraShakeTimer = 0.10f;
                    impactFlashTimer = 0.07f;
                }

                /* Attack finish */
                if (currentFrame >= swordFrames)
                {
                    isAttacking = false;

                    currentFrame = 0;
                    frameTimer = 0.0f;

                    attackLungeApplied = false;
                }
            }
        }
        /* Jump animation */
        else if (isJumping)
        {
            if (verticalVelocity < -350.0f)
            {
                currentFrame = 0;
            }
            else if (verticalVelocity < -120.0f)
            {
                currentFrame = 1;
            }
            else if (verticalVelocity < 120.0f)
            {
                currentFrame = 2;
            }
            else if (verticalVelocity < 350.0f)
            {
                currentFrame = 3;
            }
            else if (verticalVelocity < 550.0f)
            {
                currentFrame = 4;
            }
            else
            {
                currentFrame = 5;
            }
        }
        /* Idle, run and sprint animation */
        else
        {
            frameTimer += deltaTime;

            if (frameTimer >= frameDuration)
            {
                frameTimer = 0.0f;
                currentFrame++;

                if (currentFrame >= totalFrames)
                {
                    currentFrame = 0;
                }
            }
        }

        /* Screen boundaries */
        if (playerPosition.x < 0.0f)
        {
            playerPosition.x = 0.0f;
        }

        if (playerPosition.x +
                playerDrawWidth >
            screenWidth)
        {
            playerPosition.x =
                screenWidth -
                playerDrawWidth;
        }

        /* Source rectangle and horizontal flip */
        Rectangle sourceRectangle;

        if (facingRight)
        {
            sourceRectangle = (Rectangle){
                currentFrame *
                    currentFrameWidth,
                0.0f,
                currentFrameWidth,
                currentFrameHeight
            };
        }
        else
        {
            sourceRectangle = (Rectangle){
                (currentFrame + 1) *
                    currentFrameWidth,
                0.0f,
                -currentFrameWidth,
                currentFrameHeight
            };
        }

        Rectangle destinationRectangle = {
            playerPosition.x,
            playerPosition.y,
            playerDrawWidth,
            playerDrawHeight
        };

        Vector2 playerOrigin = {
            0.0f,
            0.0f
        };

        /* Attack hitbox */
        bool attackHitboxActive =
            isAttacking &&
            currentFrame >= 3 &&
            currentFrame <= 5;

        Rectangle attackHitbox = {
            0.0f,
            0.0f,
            95.0f,
            85.0f
        };

        attackHitbox.y =
            playerPosition.y +
            playerDrawHeight * 0.35f;

        if (facingRight)
        {
            attackHitbox.x =
                playerPosition.x +
                playerDrawWidth * 0.62f;
        }
        else
        {
            attackHitbox.x =
                playerPosition.x -
                attackHitbox.width * 0.55f;
        }

        BeginDrawing();

        ClearBackground(BLACK);

        /*
         * Background, effects এবং player—
         * সব world camera-এর ভেতরে।
         */
        BeginMode2D(camera);

        /* Main Gate background */
        DrawTexturePro(
            mainGateBackground,
            backgroundSource,
            backgroundDestination,
            backgroundOrigin,
            0.0f,
            WHITE
        );

        /*
         * অদৃশ্য collision ground বোঝার জন্য
         * F2 চাপলে একটি line দেখা যাবে।
         */
        if (IsKeyDown(KEY_F2))
        {
            DrawLine(
                0,
                (int)groundY,
                screenWidth,
                (int)groundY,
                YELLOW
            );
        }

        /* Sprint dust */
        if (isSprinting)
        {
            float dustX;

            if (facingRight)
            {
                dustX =
                    playerPosition.x +
                    playerDrawWidth * 0.20f;
            }
            else
            {
                dustX =
                    playerPosition.x +
                    playerDrawWidth * 0.80f;
            }

            float dustY =
                playerPosition.y +
                playerDrawHeight * 0.92f;

            DrawCircle(
                (int)dustX,
                (int)dustY,
                9.0f,
                Fade(LIGHTGRAY, 0.28f)
            );

            DrawCircle(
                (int)(
                    dustX -
                    (facingRight ?
                     13.0f :
                     -13.0f)
                ),
                (int)(dustY + 3.0f),
                6.0f,
                Fade(LIGHTGRAY, 0.20f)
            );
        }

        /* Sword slash effect */
        if (isAttacking &&
            currentFrame >= 3 &&
            currentFrame <= 5)
        {
            float direction =
                facingRight ?
                1.0f :
                -1.0f;

            Vector2 slashStart = {
                playerPosition.x +
                    playerDrawWidth * 0.52f,
                playerPosition.y +
                    playerDrawHeight * 0.48f
            };

            Vector2 slashEndOuter = {
                slashStart.x +
                    90.0f * direction,
                slashStart.y - 35.0f
            };

            Vector2 slashEndMiddle = {
                slashStart.x +
                    105.0f * direction,
                slashStart.y
            };

            Vector2 slashEndLower = {
                slashStart.x +
                    90.0f * direction,
                slashStart.y + 30.0f
            };

            DrawLineEx(
                slashStart,
                slashEndOuter,
                9.0f,
                Fade(SKYBLUE, 0.28f)
            );

            DrawLineEx(
                slashStart,
                slashEndMiddle,
                7.0f,
                Fade(BLUE, 0.42f)
            );

            DrawLineEx(
                slashStart,
                slashEndLower,
                4.0f,
                Fade(RAYWHITE, 0.70f)
            );
        }

        /* Player */
        DrawTexturePro(
            currentTexture,
            sourceRectangle,
            destinationRectangle,
            playerOrigin,
            0.0f,
            WHITE
        );

        /* Debug attack hitbox */
        if (showAttackHitbox &&
            attackHitboxActive)
        {
            DrawRectangleRec(
                attackHitbox,
                Fade(RED, 0.35f)
            );

            DrawRectangleLinesEx(
                attackHitbox,
                2.0f,
                RED
            );
        }

        EndMode2D();

        /* Impact flash */
        if (impactFlashTimer > 0.0f)
        {
            DrawRectangle(
                0,
                0,
                screenWidth,
                screenHeight,
                Fade(SKYBLUE, 0.09f)
            );
        }

        /* UI */
        DrawRectangle(
            0,
            0,
            screenWidth,
            122,
            Fade(BLACK, 0.50f)
        );

        DrawText(
            "IUT RED BOX - MAIN GATE TEST",
            260,
            20,
            28,
            WHITE
        );

        DrawText(
            "LEFT/RIGHT = Move   SHIFT = Sprint   UP = Jump",
            190,
            58,
            19,
            LIGHTGRAY
        );

        DrawText(
            "SPACE = Attack   F1 = Hitbox   Hold F2 = Ground Line",
            210,
            87,
            17,
            GRAY
        );

        DrawText(
            TextFormat(
                "State: %s",
                isAttacking ? "ATTACKING" :
                isJumping ?
                    (isSprintJump ?
                     "SPRINT JUMP" :
                     "JUMPING") :
                isSprinting ? "SPRINTING" :
                isRunning ? "RUNNING" :
                "IDLE"
            ),
            20,
            560,
            20,
            WHITE
        );

        DrawText(
            TextFormat(
                "Frame: %d",
                currentFrame
            ),
            430,
            560,
            18,
            LIGHTGRAY
        );

        float displayedSpeed;

        if (isJumping)
        {
            displayedSpeed =
                isSprintJump ?
                sprintAirSpeed :
                airSpeed;
        }
        else if (isSprinting)
        {
            displayedSpeed =
                sprintSpeed;
        }
        else
        {
            displayedSpeed =
                groundSpeed;
        }

        DrawText(
            TextFormat(
                "Speed: %.0f",
                displayedSpeed
            ),
            700,
            560,
            18,
            (isSprinting ||
             isSprintJump) ?
                YELLOW :
                LIGHTGRAY
        );

        EndDrawing();
    }

    /* Texture unload */
    UnloadTexture(mainGateBackground);
    UnloadTexture(idleTexture);
    UnloadTexture(runTexture);
    UnloadTexture(jumpTexture);
    UnloadTexture(swordTexture);

    CloseWindow();

    return 0;
}