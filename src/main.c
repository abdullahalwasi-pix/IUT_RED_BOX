#include "raylib.h"

int main(void)
{
    const int screenWidth = 1000;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "IUT Red Box");
    SetTargetFPS(60);

    Texture2D idleTexture = LoadTexture("../assets/player/idle.png");
    Texture2D runTexture = LoadTexture("../assets/player/run.png");

    if (!IsTextureValid(idleTexture) || !IsTextureValid(runTexture))
    {
        TraceLog(LOG_ERROR, "Player texture load failed.");
        CloseWindow();
        return 1;
    }

    const int idleFrames = 8;
    const int runFrames = 8;

    float idleFrameWidth = (float)idleTexture.width / idleFrames;
    float idleFrameHeight = (float)idleTexture.height;

    float runFrameWidth = (float)runTexture.width / runFrames;
    float runFrameHeight = (float)runTexture.height;

    int currentFrame = 0;
    float frameTimer = 0.0f;
    float frameDuration = 0.12f;

    Vector2 playerPosition = {
        350.0f,
        100.0f
    };

    float playerSpeed = 220.0f;

    bool isRunning = false;
    bool wasRunning = false;
    bool facingRight = true;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        isRunning = false;

        if (IsKeyDown(KEY_RIGHT))
{
    playerPosition.x += playerSpeed * deltaTime;
    isRunning = true;
    facingRight = true;
}

if (IsKeyDown(KEY_LEFT))
{
    playerPosition.x -= playerSpeed * deltaTime;
    isRunning = true;
    facingRight = false;
}

        if (isRunning != wasRunning)
        {
            currentFrame = 0;
            frameTimer = 0.0f;
            wasRunning = isRunning;
        }

        frameTimer += deltaTime;

        if (frameTimer >= frameDuration)
        {
            frameTimer = 0.0f;
            currentFrame++;

            if (isRunning)
            {
                if (currentFrame >= runFrames)
                {
                    currentFrame = 0;
                }
            }
            else
            {
                if (currentFrame >= idleFrames)
                {
                    currentFrame = 0;
                }
            }
        }

        Texture2D currentTexture;
        float currentFrameWidth;
        float currentFrameHeight;
        float drawScale;

        if (isRunning)
        {
            currentTexture = runTexture;
            currentFrameWidth = runFrameWidth;
            currentFrameHeight = runFrameHeight;
            drawScale = 0.72f;
        }
        else
        {
            currentTexture = idleTexture;
            currentFrameWidth = idleFrameWidth;
            currentFrameHeight = idleFrameHeight;
            drawScale = 0.52f;
        }

        float playerDrawWidth = currentFrameWidth * drawScale;
        float playerDrawHeight = currentFrameHeight * drawScale;

        if (playerPosition.x < 0.0f)
        {
            playerPosition.x = 0.0f;
        }

        if (playerPosition.x + playerDrawWidth > screenWidth)
        {
            playerPosition.x = screenWidth - playerDrawWidth;
        }

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
        (currentFrame + 1) * currentFrameWidth,
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

        Vector2 origin = {0.0f, 0.0f};

        BeginDrawing();

        ClearBackground(DARKGRAY);

        DrawText(
            "IUT RED BOX - MOVEMENT TEST",
            240,
            30,
            30,
            WHITE
        );

        DrawText(
            "LEFT ARROW = Left    RIGHT ARROW = Right",
            270,
            70,
            20,
            LIGHTGRAY
        );

        DrawTexturePro(
            currentTexture,
            sourceRectangle,
            destinationRectangle,
            origin,
            0.0f,
            WHITE
        );

        DrawText(
            TextFormat(
                "State: %s",
                isRunning ? "RUNNING" : "IDLE"
            ),
            20,
            540,
            20,
            WHITE
        );

        EndDrawing();
    }

    UnloadTexture(idleTexture);
    UnloadTexture(runTexture);

    CloseWindow();

    return 0;
}