#include "raylib.h"

int main(void)
{
    const int screenWidth = 1000;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "IUT Red Box");
    SetTargetFPS(60);

    /* Player texture load */
    Texture2D idleTexture =
        LoadTexture("../assets/player/idle.png");

    Texture2D runTexture =
        LoadTexture("../assets/player/run.png");

    Texture2D jumpTexture =
        LoadTexture("../assets/player/jump.png");

    Texture2D swordTexture =
        LoadTexture("../assets/player/sword.png");

    /* Texture validation */
    if (!IsTextureValid(idleTexture) ||
        !IsTextureValid(runTexture) ||
        !IsTextureValid(jumpTexture) ||
        !IsTextureValid(swordTexture))
    {
        TraceLog(LOG_ERROR, "Player texture load failed.");

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

    /* Animation frame সংখ্যা */
    const int idleFrames = 8;
    const int runFrames = 8;
    const int jumpFrames = 6;
    const int swordFrames = 8;

    /* Frame size */
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

    /* Animation variables */
    int currentFrame = 0;
    float frameTimer = 0.0f;

    /* Player position */
    Vector2 playerPosition = {
        350.0f,
        100.0f
    };

    /* Movement */
    float groundSpeed = 220.0f;
    float airSpeed = 330.0f;

    /* Jump physics */
    float verticalVelocity = 0.0f;
    float jumpForce = -800.0f;
float gravity = 2000.0f;

    /* Ground */
    float groundY = 420.0f;

    /* Player states */
    bool isRunning = false;
    bool wasRunning = false;

    bool isJumping = false;
    bool wasJumping = false;

    bool isAttacking = false;
    bool facingRight = true;

    /*
     * Attack effect variables
     */
    bool attackLungeApplied = false;

    float cameraShakeTimer = 0.0f;
    float impactFlashTimer = 0.0f;

    bool showAttackHitbox = false;

    /*
     * Camera শুধু screen shake-এর জন্য।
     */
    Camera2D camera = {0};

    camera.target = (Vector2){0.0f, 0.0f};
    camera.offset = (Vector2){0.0f, 0.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        isRunning = false;

        /*
         * F1 দিয়ে attack hitbox দেখা বা লুকানো যাবে।
         */
        if (IsKeyPressed(KEY_F1))
        {
            showAttackHitbox = !showAttackHitbox;
        }

        /*
         * Ground attack শুরু।
         */
        if (IsKeyPressed(KEY_SPACE) &&
            !isAttacking &&
            !isJumping)
        {
            isAttacking = true;

            currentFrame = 0;
            frameTimer = 0.0f;

            attackLungeApplied = false;
        }

        /*
         * Attack চলার সময় jump করা যাবে না।
         */
        if (IsKeyPressed(KEY_UP) &&
            !isJumping &&
            !isAttacking)
        {
            verticalVelocity = jumpForce;
            isJumping = true;

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        /*
         * Ground ও air movement speed।
         */
        float horizontalSpeed;

        if (isJumping)
        {
            horizontalSpeed = airSpeed;
        }
        else
        {
            horizontalSpeed = groundSpeed;
        }

        /*
         * Attack চলাকালে সাধারণ movement বন্ধ।
         */
        if (!isAttacking)
        {
            if (IsKeyDown(KEY_RIGHT))
            {
                playerPosition.x +=
                    horizontalSpeed * deltaTime;

                isRunning = true;
                facingRight = true;
            }

            if (IsKeyDown(KEY_LEFT))
            {
                playerPosition.x -=
                    horizontalSpeed * deltaTime;

                isRunning = true;
                facingRight = false;
            }
        }

        /*
         * Variable jump height।
         */
        if (isJumping &&
            IsKeyReleased(KEY_UP) &&
            verticalVelocity < -200.0f)
        {
            verticalVelocity *= 0.55f;
        }

        /*
         * Gravity।
         */
        if (isJumping)
        {
            verticalVelocity += gravity * deltaTime;

            playerPosition.y +=
                verticalVelocity * deltaTime;
        }

        /*
         * Screen shake timer update।
         */
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

        /*
         * Impact flash timer update।
         */
        if (impactFlashTimer > 0.0f)
        {
            impactFlashTimer -= deltaTime;
        }

        /*
         * বর্তমান animation state-এর texture ও size।
         */
        Texture2D currentTexture;

        float currentFrameWidth;
        float currentFrameHeight;
        float drawScale;
        float frameDuration;

        int totalFrames;

        /*
         * State priority:
         * Attack → Jump → Run → Idle
         */
        if (isAttacking)
        {
            currentTexture = swordTexture;

            currentFrameWidth = swordFrameWidth;
            currentFrameHeight = swordFrameHeight;

            /*
             * Sword sprite-এর visual size।
             * বেশি বড় হলে 0.60f করা যাবে।
             */
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
            frameDuration = 0.12f;
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

        /*
         * Ground alignment।
         */
        if (!isJumping)
        {
            playerPosition.y = playerGroundPosition;
        }

        /*
         * Landing।
         */
        if (isJumping &&
            playerPosition.y >= playerGroundPosition &&
            verticalVelocity > 0.0f)
        {
            playerPosition.y = playerGroundPosition;

            verticalVelocity = 0.0f;
            isJumping = false;

            currentFrame = 0;
            frameTimer = 0.0f;
        }

        /*
         * State বদলালে animation reset।
         */
        if (!isAttacking &&
            (isJumping != wasJumping ||
             (!isJumping &&
              isRunning != wasRunning)))
        {
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        wasJumping = isJumping;
        wasRunning = isRunning;

        /*
         * Professional sword attack timing।
         */
        if (isAttacking)
        {
            /*
             * প্রতিটি phase-এর speed আলাদা।
             *
             * 0–1 = anticipation
             * 2–3 = fast swing
             * 4   = impact
             * 5–7 = recovery
             */
            float currentAttackFrameDuration;

            if (currentFrame <= 1)
            {
                currentAttackFrameDuration = 0.09f;
            }
            else if (currentFrame <= 3)
            {
                currentAttackFrameDuration = 0.055f;
            }
            else if (currentFrame == 4)
            {
                /*
                 * Impact frame একটু বেশি সময় থাকবে।
                 */
                currentAttackFrameDuration = 0.14f;
            }
            else
            {
                currentAttackFrameDuration = 0.08f;
            }

            frameTimer += deltaTime;

            if (frameTimer >=
                currentAttackFrameDuration)
            {
                frameTimer = 0.0f;
                currentFrame++;

                /*
                 * Swing শুরু হলে player সামান্য সামনে যাবে।
                 */
                if (currentFrame == 3 &&
                    !attackLungeApplied)
                {
                    float lungeDistance = 24.0f;

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

                /*
                 * Impact frame-এ shake ও flash।
                 */
                if (currentFrame == 4)
                {
                    cameraShakeTimer = 0.10f;
                    impactFlashTimer = 0.07f;
                }

                /*
                 * Attack শেষ।
                 */
                if (currentFrame >= swordFrames)
                {
                    isAttacking = false;

                    currentFrame = 0;
                    frameTimer = 0.0f;

                    attackLungeApplied = false;
                }
            }
        }
        /*
         * Jump animation physics অনুযায়ী।
         */
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
        /*
         * Idle ও run animation loop।
         */
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

        /*
         * Screen boundary।
         */
        if (playerPosition.x < 0.0f)
        {
            playerPosition.x = 0.0f;
        }

        if (playerPosition.x + playerDrawWidth >
            screenWidth)
        {
            playerPosition.x =
                screenWidth - playerDrawWidth;
        }

        /*
         * Source rectangle ও direction flip।
         */
        Rectangle sourceRectangle;

        if (facingRight)
        {
            sourceRectangle = (Rectangle){
                currentFrame * currentFrameWidth,
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

        Vector2 origin = {
            0.0f,
            0.0f
        };

        /*
         * Attack hitbox।
         * Frame 3, 4 ও 5-এ active থাকবে।
         */
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

        ClearBackground(DARKGRAY);

        /*
         * Camera shake শুধু game world-এ।
         * UI shake করবে না।
         */
        BeginMode2D(camera);

        DrawRectangle(
            0,
            (int)groundY,
            screenWidth,
            screenHeight - (int)groundY,
            DARKGREEN
        );

        /*
         * Slash trail effect।
         */
        if (isAttacking &&
            currentFrame >= 3 &&
            currentFrame <= 5)
        {
            float direction;

            if (facingRight)
            {
                direction = 1.0f;
            }
            else
            {
                direction = -1.0f;
            }

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

            /*
             * তিন স্তরের slash line।
             */
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

        /*
         * Player draw।
         */
        DrawTexturePro(
            currentTexture,
            sourceRectangle,
            destinationRectangle,
            origin,
            0.0f,
            WHITE
        );

        /*
         * F1 চালু থাকলে hitbox দেখা যাবে।
         */
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

        /*
         * Impact flash।
         */
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

        DrawText(
            "IUT RED BOX - PROFESSIONAL COMBAT TEST",
            175,
            25,
            28,
            WHITE
        );

        DrawText(
            "LEFT/RIGHT = Move   UP = Jump   SPACE = Attack",
            210,
            65,
            20,
            LIGHTGRAY
        );

        DrawText(
            "F1 = Show/Hide Attack Hitbox",
            340,
            95,
            18,
            GRAY
        );

        DrawText(
            TextFormat(
                "State: %s",
                isAttacking ? "ATTACKING" :
                isJumping ? "JUMPING" :
                isRunning ? "RUNNING" :
                "IDLE"
            ),
            20,
            550,
            20,
            WHITE
        );

        DrawText(
            TextFormat(
                "Frame: %d",
                currentFrame
            ),
            450,
            550,
            18,
            LIGHTGRAY
        );

        DrawText(
            TextFormat(
                "Hitbox: %s",
                attackHitboxActive ?
                    "ACTIVE" :
                    "INACTIVE"
            ),
            700,
            550,
            18,
            attackHitboxActive ?
                YELLOW :
                LIGHTGRAY
        );

        EndDrawing();
    }

    UnloadTexture(idleTexture);
    UnloadTexture(runTexture);
    UnloadTexture(jumpTexture);
    UnloadTexture(swordTexture);

    CloseWindow();

    return 0;
}