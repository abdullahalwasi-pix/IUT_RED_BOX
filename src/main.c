#include "raylib.h"
#include "config.h"
#include "game.h"

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "IUT Red Box - Level 1 Zomvsbie Student"
    );

    ToggleBorderlessWindowed();

    InitAudioDevice();
    SetTargetFPS(60);

    Game game = {0};

    if (!InitGame(&game))
    {
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    while (!WindowShouldClose())
    {
        UpdateGame(&game);
        DrawGame(&game);
    }

    UnloadGame(&game);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}
