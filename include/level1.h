#ifndef LEVEL1_H
#define LEVEL1_H

#include "raylib.h"
#include <stdbool.h>

typedef enum
{
    LEVEL1_OPENING,
    LEVEL1_AREA_INTRO,
    LEVEL1_TUTORIAL,
    LEVEL1_COMBAT,
    LEVEL1_POST_FIGHT,
    LEVEL1_CLUE,
    LEVEL1_EXIT_READY,
    LEVEL1_COMPLETE
} Level1State;

typedef struct
{
    Level1State state;

    float stateTimer;
    float messageTimer;

    int openingTextIndex;
    int tutorialStep;
    int dialogueIndex;

    bool moved;
    bool jumped;
    bool attacked;
    bool enemyDefeated;
    bool dialogueFinished;
    bool clueCollected;
    bool exitReached;
} Level1;

void InitLevel1(Level1 *level);
void ResetLevel1(Level1 *level);

void UpdateLevel1(
    Level1 *level,
    float dt,
    bool enemyDefeated
);

bool Level1AllowsPlayerControl(const Level1 *level);
bool Level1AllowsEnemyAI(const Level1 *level);
bool Level1ShowsGameplay(const Level1 *level);
bool Level1ShowsHud(const Level1 *level);
bool Level1ShowsBattleVictoryPose(const Level1 *level);

void DrawLevel1Overlay(
    const Level1 *level,
    Texture2D campusDestroyed,
    Texture2D missionPaperScene,
    Texture2D redBoxLake
);

#endif