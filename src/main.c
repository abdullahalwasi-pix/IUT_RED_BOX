#include "raylib.h"

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "2D Action Game");
    SetTargetFPS(60);

    Texture2D idle = LoadTexture("assets/idle.png");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawTexture(idle, 100, 100, WHITE);

        EndDrawing();
    }

    UnloadTexture(idle);

    CloseWindow();

    return 0;
}