#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "player.h"
#include "enemy.h"
#include <stdbool.h>

typedef enum
{
    GAME_PLAYING,
    GAME_VICTORY,
    GAME_DEFEAT
} GameState;

typedef struct
{
    Sound swordSwing;
    Sound swordHit;
    Sound playerHit;
    Sound enemyTelegraph;
    Sound enemyPush;
    Sound victory;
    Sound defeat;
    Sound level1Intro;
    Music level1Ambience;

    bool swordSwingReady;
    bool swordHitReady;
    bool playerHitReady;
    bool enemyTelegraphReady;
    bool enemyPushReady;
    bool victoryReady;
    bool defeatReady;
    bool level1IntroReady;
    bool level1AmbienceReady;
} GameAudio;

typedef struct
{
    Texture2D background;
    RenderTexture2D target;

    Rectangle backgroundSource;
    Rectangle backgroundDestination;

    PlayerAssets playerAssets;
    ZombieAssets zombieAssets;

    Player player;
    ZombieEnemy enemy;
    LevelEnemyConfig levelConfig;

    GameAudio audio;
    Camera2D camera;
    GameState state;

    float groundY;
    float resultTimer;
    float levelIntroTimer;
    float cameraShakeTimer;
    float impactFlashTimer;

    int successfulHits;

    bool showHitboxes;
    bool ambienceStarted;
    bool victorySoundPlayed;
    bool defeatSoundPlayed;
} Game;

bool InitGame(Game *game);
void RestartGame(Game *game);
void UpdateGame(Game *game);
void DrawGame(Game *game);
void UnloadGame(Game *game);

#endif
