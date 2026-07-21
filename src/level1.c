#include "level1.h"
#include "config.h"
#include "raylib.h"
#include <math.h>

/*
   OPENING SCENE ORDER
   0 = Black title card
   1 = Destroyed IUT aerial cinematic
   2 = Mission paper briefing
   3 = Final Red Box warning
*/
#define OPENING_SCENE_COUNT 4
#define TITLE_SCENE_DURATION 6.20f
#define CAMPUS_SCENE_DURATION 8.80f
#define PAPER_SCENE_DURATION 12.00f
#define WARNING_SCENE_DURATION 7.00f
#define AREA_INTRO_DURATION 4.80f
#define FADE_DURATION 0.55f

/* Edit these strings whenever you want to change the story text. */
static const char *MISSION_TITLE =
    "IUT EMERGENCY RECOVERY DIRECTIVE";

static const char *MISSION_SUBTITLE =
    "PROTOCOL: RED BOX";

static const char *MISSION_LINES[] =
{
    "1. Assess the Main Gate",
    "2. Restore corrupted department systems",
    "3. Recover the Red Fragments",
    "4. Reach the recovery system at IUT Lake",
    "5. Complete the Final Debug"
};

static const int MISSION_LINE_COUNT =
    (int)(sizeof(MISSION_LINES) / sizeof(MISSION_LINES[0]));


static const char *POST_FIGHT_DIALOGUE[] =
{
    "This student was not acting on his own...",
    "The corruption is controlling the campus network.",
    "Every department may be connected to the same signal.",
    "I need to trace its source before the system spreads."
};

static const int POST_FIGHT_DIALOGUE_COUNT =
    (int)(
        sizeof(POST_FIGHT_DIALOGUE) /
        sizeof(POST_FIGHT_DIALOGUE[0])
    );

static float ClampFloat(
    float value,
    float minimum,
    float maximum
)
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

static float SmoothStep01(float value)
{
    value = ClampFloat(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}

static float GetSceneDuration(int sceneIndex)
{
    switch (sceneIndex)
    {
        case 0:
            return TITLE_SCENE_DURATION;

        case 1:
            return CAMPUS_SCENE_DURATION;

        case 2:
            return PAPER_SCENE_DURATION;

        default:
            return WARNING_SCENE_DURATION;
    }
}

static float GetFadeAlpha(
    float timer,
    float totalDuration
)
{
    float alpha = 1.0f;

    if (timer < FADE_DURATION)
    {
        alpha = timer / FADE_DURATION;
    }
    else if (timer > totalDuration - FADE_DURATION)
    {
        alpha =
            (totalDuration - timer) /
            FADE_DURATION;
    }

    return ClampFloat(alpha, 0.0f, 1.0f);
}

static void DrawCenteredText(
    const char *text,
    int y,
    int fontSize,
    Color color
)
{
    int textWidth = MeasureText(text, fontSize);

    DrawText(
        text,
        SCREEN_WIDTH / 2 - textWidth / 2,
        y,
        fontSize,
        color
    );
}

static void DrawTextureCover(
    Texture2D texture,
    float zoom,
    Color tint
)
{
    if (!IsTextureValid(texture))
    {
        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            BLACK
        );
        return;
    }

    float sourceAspect =
        (float)texture.width /
        (float)texture.height;

    float targetAspect =
        (float)SCREEN_WIDTH /
        (float)SCREEN_HEIGHT;

    Rectangle source = {
        0.0f,
        0.0f,
        (float)texture.width,
        (float)texture.height
    };

    if (sourceAspect > targetAspect)
    {
        float wantedWidth =
            (float)texture.height *
            targetAspect;

        source.x =
            ((float)texture.width - wantedWidth) *
            0.5f;

        source.width = wantedWidth;
    }
    else
    {
        float wantedHeight =
            (float)texture.width /
            targetAspect;

        source.y =
            ((float)texture.height - wantedHeight) *
            0.5f;

        source.height = wantedHeight;
    }

    float destinationWidth =
        (float)SCREEN_WIDTH * zoom;

    float destinationHeight =
        (float)SCREEN_HEIGHT * zoom;

    Rectangle destination = {
        ((float)SCREEN_WIDTH - destinationWidth) * 0.5f,
        ((float)SCREEN_HEIGHT - destinationHeight) * 0.5f,
        destinationWidth,
        destinationHeight
    };

    DrawTexturePro(
        texture,
        source,
        destination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        tint
    );
}

static void DrawLetterboxBars(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, 52, BLACK);
    DrawRectangle(
        0,
        SCREEN_HEIGHT - 52,
        SCREEN_WIDTH,
        52,
        BLACK
    );
}

static void DrawSoftVignette(void)
{
    DrawRectangleGradientH(
        0,
        0,
        135,
        SCREEN_HEIGHT,
        Fade(BLACK, 0.78f),
        BLANK
    );

    DrawRectangleGradientH(
        SCREEN_WIDTH - 135,
        0,
        135,
        SCREEN_HEIGHT,
        BLANK,
        Fade(BLACK, 0.78f)
    );

    DrawRectangleGradientV(
        0,
        0,
        SCREEN_WIDTH,
        95,
        Fade(BLACK, 0.62f),
        BLANK
    );

    DrawRectangleGradientV(
        0,
        SCREEN_HEIGHT - 120,
        SCREEN_WIDTH,
        120,
        BLANK,
        Fade(BLACK, 0.72f)
    );
}

static void StartAreaIntroduction(Level1 *level)
{
    level->state = LEVEL1_AREA_INTRO;
    level->stateTimer = 0.0f;
}

static void StartTutorial(Level1 *level)
{
    level->state = LEVEL1_TUTORIAL;
    level->stateTimer = 0.0f;
    level->messageTimer = 0.0f;
    level->tutorialStep = 0;
    level->moved = false;
    level->jumped = false;
    level->attacked = false;
}

static void AdvanceOpeningScene(Level1 *level)
{
    level->openingTextIndex++;
    level->stateTimer = 0.0f;

    if (level->openingTextIndex >= OPENING_SCENE_COUNT)
    {
        StartAreaIntroduction(level);
    }
}

void InitLevel1(Level1 *level)
{
    ResetLevel1(level);
}

void ResetLevel1(Level1 *level)
{
    if (level == 0)
    {
        return;
    }

    *level = (Level1){0};
    level->state = LEVEL1_OPENING;
    level->openingTextIndex = 0;
}

void UpdateLevel1(
    Level1 *level,
    float dt,
    bool enemyDefeated
)
{
    if (level == 0)
    {
        return;
    }

    level->stateTimer += dt;
    level->enemyDefeated = enemyDefeated;

    if (level->state == LEVEL1_OPENING)
    {
        float sceneDuration =
            GetSceneDuration(
                level->openingTextIndex
            );

        /* ENTER advances one scene. SPACE skips the full opening. */
        if (IsKeyPressed(KEY_SPACE))
        {
            StartAreaIntroduction(level);
            return;
        }

        if (
            IsKeyPressed(KEY_ENTER) ||
            level->stateTimer >= sceneDuration
        )
        {
            AdvanceOpeningScene(level);
        }

        return;
    }

    if (level->state == LEVEL1_AREA_INTRO)
    {
        if (
            IsKeyPressed(KEY_ENTER) ||
            level->stateTimer >= AREA_INTRO_DURATION
        )
        {
            StartTutorial(level);
        }

        return;
    }

    if (level->state == LEVEL1_TUTORIAL)
    {
        /*
           Current game controls:
           LEFT / RIGHT = move
           UP           = jump
           SPACE        = attack
        */

        if (
            level->tutorialStep == 0 &&
            (
                IsKeyDown(KEY_LEFT) ||
                IsKeyDown(KEY_RIGHT)
            )
        )
        {
            level->moved = true;
            level->tutorialStep = 1;
            level->stateTimer = 0.0f;
        }
        else if (
            level->tutorialStep == 1 &&
            IsKeyPressed(KEY_UP)
        )
        {
            level->jumped = true;
            level->tutorialStep = 2;
            level->stateTimer = 0.0f;
        }
        else if (
            level->tutorialStep == 2 &&
            IsKeyPressed(KEY_SPACE)
        )
        {
            level->attacked = true;
            level->tutorialStep = 3;
            level->stateTimer = 0.0f;
        }
        else if (
            level->tutorialStep == 3 &&
            level->stateTimer >= 1.60f
        )
        {
            level->state = LEVEL1_COMBAT;
            level->stateTimer = 0.0f;
            level->messageTimer = 2.80f;
        }

        return;
    }

    if (level->state == LEVEL1_COMBAT)
    {
        if (level->messageTimer > 0.0f)
        {
            level->messageTimer -= dt;

            if (level->messageTimer < 0.0f)
            {
                level->messageTimer = 0.0f;
            }
        }

        if (enemyDefeated)
        {
            level->state = LEVEL1_POST_FIGHT;
            level->stateTimer = 0.0f;
            level->dialogueIndex = 0;
            level->dialogueFinished = false;
        }

        return;
    }

    if (level->state == LEVEL1_POST_FIGHT)
    {
        if (
            !level->dialogueFinished &&
            IsKeyPressed(KEY_ENTER)
        )
        {
            level->dialogueIndex++;
            level->stateTimer = 0.0f;

            if (
                level->dialogueIndex >=
                POST_FIGHT_DIALOGUE_COUNT
            )
            {
                level->dialogueIndex =
                    POST_FIGHT_DIALOGUE_COUNT - 1;

                level->dialogueFinished = true;
            }
        }

        return;
    }
}

bool Level1AllowsPlayerControl(const Level1 *level)
{
    if (level == 0)
    {
        return false;
    }

    return level->state == LEVEL1_TUTORIAL ||
           level->state == LEVEL1_COMBAT ||
           level->state == LEVEL1_CLUE ||
           level->state == LEVEL1_EXIT_READY;
}

bool Level1AllowsEnemyAI(const Level1 *level)
{
    if (level == 0)
    {
        return false;
    }

    return level->state == LEVEL1_COMBAT;
}

bool Level1ShowsGameplay(const Level1 *level)
{
    if (level == 0)
    {
        return false;
    }

    return level->state != LEVEL1_OPENING &&
           level->state != LEVEL1_AREA_INTRO;
}

bool Level1ShowsHud(const Level1 *level)
{
    return Level1ShowsGameplay(level);
}

static void DrawTitleScene(
    const Level1 *level,
    float alpha
)
{
    (void)level;

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        BLACK
    );

    DrawCenteredText(
        "IUT RED BOX",
        206,
        48,
        Fade(RED, alpha)
    );

    DrawRectangle(
        SCREEN_WIDTH / 2 - 145,
        273,
        290,
        3,
        Fade(RED, 0.85f * alpha)
    );

    DrawCenteredText(
        "A normal day at IUT turned into a nightmare...",
        306,
        22,
        Fade(RAYWHITE, alpha)
    );
}

static void DrawCampusScene(
    const Level1 *level,
    Texture2D campusDestroyed,
    float alpha
)
{
    float progress = ClampFloat(
        level->stateTimer /
            CAMPUS_SCENE_DURATION,
        0.0f,
        1.0f
    );

    const float revealStart = 0.25f;
    const float revealDuration = 1.35f;
    const float textStart = 1.10f;
    const float textFadeDuration = 0.85f;

    float revealProgress = SmoothStep01(
        (level->stateTimer - revealStart) /
        revealDuration
    );

    float textAlpha = SmoothStep01(
        (level->stateTimer - textStart) /
        textFadeDuration
    ) * alpha;

    DrawTextureCover(
        campusDestroyed,
        1.0f + progress * 0.045f,
        Fade(WHITE, alpha)
    );

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade((Color){55, 0, 0, 255}, 0.14f * alpha)
    );

    DrawSoftVignette();

    float halfGap =
        6.0f +
        revealProgress *
        ((float)SCREEN_WIDTH * 0.50f + 8.0f);

    int leftMaskWidth =
        (int)((float)SCREEN_WIDTH * 0.50f - halfGap);

    int rightMaskX =
        (int)((float)SCREEN_WIDTH * 0.50f + halfGap);

    if (leftMaskWidth > 0)
    {
        DrawRectangle(
            0,
            0,
            leftMaskWidth,
            SCREEN_HEIGHT,
            BLACK
        );
    }

    if (rightMaskX < SCREEN_WIDTH)
    {
        DrawRectangle(
            rightMaskX,
            0,
            SCREEN_WIDTH - rightMaskX,
            SCREEN_HEIGHT,
            BLACK
        );
    }

    if (revealProgress < 1.0f)
    {
        const int edgeSoftness = 58;

        DrawRectangleGradientH(
            leftMaskWidth,
            0,
            edgeSoftness,
            SCREEN_HEIGHT,
            Fade(BLACK, 0.90f),
            BLANK
        );

        DrawRectangleGradientH(
            rightMaskX - edgeSoftness,
            0,
            edgeSoftness,
            SCREEN_HEIGHT,
            BLANK,
            Fade(BLACK, 0.90f)
        );
    }

    DrawLetterboxBars();

    if (textAlpha > 0.0f)
    {
        DrawCenteredText(
            "Campus systems collapsed without warning.",
            446,
            22,
            Fade(RAYWHITE, textAlpha)
        );

        DrawCenteredText(
            "Every department fell to corruption.",
            478,
            20,
            Fade(LIGHTGRAY, textAlpha)
        );

        DrawCenteredText(
            "Everyone disappeared. Only one professor remained.",
            510,
            20,
            Fade(LIGHTGRAY, textAlpha)
        );
    }
}

static void DrawTextCenteredInPaper(
    const char *text,
    int paperCenterX,
    int y,
    int fontSize,
    Color color
)
{
    int width = MeasureText(text, fontSize);

    DrawText(
        text,
        paperCenterX - width / 2,
        y,
        fontSize,
        color
    );
}

static void DrawMissionText(float alpha)
{
    /*
       Tuned for the final blank scroll image.
       All text stays inside the parchment safe area.
    */
    const int paperLeft = 330;
    const int paperRight = 720;
    const int paperCenter = (paperLeft + paperRight) / 2;
    const int titleY = 188;
    const int titleSize = 15;
    const int subtitleSize = 15;
    const int bodySize = 14;
    const int lineGap = 29;

    Color ink = Fade((Color){48, 29, 18, 255}, alpha);
    Color darkRed = Fade((Color){119, 25, 20, 255}, alpha);
    Color rule = Fade((Color){94, 55, 35, 255}, 0.84f * alpha);

    DrawTextCenteredInPaper(
        MISSION_TITLE,
        paperCenter - 12,
        titleY + 4,
        titleSize,
        darkRed
    );

    DrawTextCenteredInPaper(
        MISSION_SUBTITLE,
        paperCenter,
        titleY + 28,
        subtitleSize,
        ink
    );

    DrawLine(
        paperLeft + 68,
        titleY + 53,
        paperRight - 68,
        titleY + 53,
        rule
    );

    for (int i = 0; i < MISSION_LINE_COUNT; i++)
    {
        DrawText(
            MISSION_LINES[i],
            paperLeft + 18,
            titleY + 73 + i * lineGap,
            bodySize,
            ink
        );
    }

    DrawLine(
        paperLeft + 28,
        titleY + 231,
        paperRight - 28,
        titleY + 231,
        rule
    );

    DrawTextCenteredInPaper(
        "CAMPUS STABILITY: 10%",
        paperCenter,
        titleY + 245,
        15,
        darkRed
    );

    DrawTextCenteredInPaper(
        "STATUS: CRITICAL",
        paperCenter,
        titleY + 270,
        15,
        darkRed
    );
}

static void DrawPaperScene(
    const Level1 *level,
    Texture2D missionPaperScene,
    float alpha
)
{
    float progress = ClampFloat(
        level->stateTimer /
            PAPER_SCENE_DURATION,
        0.0f,
        1.0f
    );

    /*
       Eye-opening reveal:
       0.00s - 0.35s : almost fully black
       0.35s - 1.55s : eyelids open smoothly
       1.55s onward  : full paper scene visible
    */
    const float revealStart = 0.35f;
    const float revealDuration = 1.20f;
    float revealProgress = SmoothStep01(
        (level->stateTimer - revealStart) /
        revealDuration
    );

    DrawTextureCover(
        missionPaperScene,
        1.0f + progress * 0.018f,
        Fade(WHITE, alpha)
    );

    /*
       Slight darkness while the professor's eyes adjust.
       It fades away as the eyelids open.
    */
    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade(
            BLACK,
            (0.30f * (1.0f - revealProgress)) + 0.05f
        )
    );

    /*
       Two black masks simulate upper and lower eyelids.
       The visible center gap grows smoothly.
    */
    float halfGap =
        6.0f +
        revealProgress *
        ((float)SCREEN_HEIGHT * 0.50f + 8.0f);

    int upperMaskHeight =
        (int)((float)SCREEN_HEIGHT * 0.50f - halfGap);

    int lowerMaskY =
        (int)((float)SCREEN_HEIGHT * 0.50f + halfGap);

    if (upperMaskHeight > 0)
    {
        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            upperMaskHeight,
            BLACK
        );
    }

    if (lowerMaskY < SCREEN_HEIGHT)
    {
        DrawRectangle(
            0,
            lowerMaskY,
            SCREEN_WIDTH,
            SCREEN_HEIGHT - lowerMaskY,
            BLACK
        );
    }

    /*
       Soft shadow near the eyelid edges.
       This makes the reveal feel less mechanical.
    */
    if (revealProgress < 1.0f)
    {
        const int edgeSoftness = 54;

        DrawRectangleGradientV(
            0,
            upperMaskHeight,
            SCREEN_WIDTH,
            edgeSoftness,
            Fade(BLACK, 0.88f),
            BLANK
        );

        DrawRectangleGradientV(
            0,
            lowerMaskY - edgeSoftness,
            SCREEN_WIDTH,
            edgeSoftness,
            BLANK,
            Fade(BLACK, 0.88f)
        );
    }

    /*
       The mission writing is present from the beginning.
       The eyelid masks reveal the paper and writing together.
    */
    DrawMissionText(alpha);

    DrawLetterboxBars();
}

static void DrawWarningScene(
    const Level1 *level,
    Texture2D redBoxLake,
    float alpha
)
{
    float progress = ClampFloat(
        level->stateTimer / WARNING_SCENE_DURATION,
        0.0f,
        1.0f
    );

    float pulse =
        0.70f +
        0.30f *
        (0.5f + 0.5f * sinf((float)GetTime() * 3.2f));

    DrawTextureCover(
        redBoxLake,
        1.0f + progress * 0.025f,
        Fade(WHITE, alpha)
    );

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade((Color){30, 0, 0, 255}, 0.18f * alpha)
    );

    DrawSoftVignette();
    DrawLetterboxBars();

    DrawRectangle(
        0,
        330,
        SCREEN_WIDTH,
        130,
        Fade(BLACK, 0.55f * alpha)
    );

    DrawCenteredText(
        "THE RED BOX HOLDS THE SOLUTION",
        354,
        28,
        Fade(RED, pulse * alpha)
    );

    DrawCenteredText(
        "Reach IUT Lake before the corruption becomes permanent.",
        402,
        19,
        Fade(RAYWHITE, alpha)
    );
}

static void DrawTutorialPanel(
    const char *label,
    const char *instruction,
    const char *progressText
)
{
    const int panelWidth = 470;
    const int panelHeight = 92;
    const int panelX =
        SCREEN_WIDTH / 2 - panelWidth / 2;
    const int panelY =
        SCREEN_HEIGHT - panelHeight - 30;

    DrawRectangle(
        panelX,
        panelY,
        panelWidth,
        panelHeight,
        Fade(BLACK, 0.82f)
    );

    DrawRectangleLinesEx(
        (Rectangle){
            (float)panelX,
            (float)panelY,
            (float)panelWidth,
            (float)panelHeight
        },
        2.0f,
        Fade(RED, 0.90f)
    );

    DrawRectangle(
        panelX,
        panelY,
        7,
        panelHeight,
        RED
    );

    DrawText(
        label,
        panelX + 28,
        panelY + 15,
        18,
        RED
    );

    DrawText(
        instruction,
        panelX + 28,
        panelY + 43,
        23,
        RAYWHITE
    );

    DrawText(
        progressText,
        panelX + panelWidth - 86,
        panelY + 17,
        16,
        LIGHTGRAY
    );
}

static void DrawTutorialOverlay(
    const Level1 *level
)
{
    if (level->tutorialStep == 0)
    {
        DrawTutorialPanel(
            "MOVEMENT TRAINING",
            "Use LEFT / RIGHT to move",
            "1 / 3"
        );
        return;
    }

    if (level->tutorialStep == 1)
    {
        DrawTutorialPanel(
            "JUMP TRAINING",
            "Press UP to jump",
            "2 / 3"
        );
        return;
    }

    if (level->tutorialStep == 2)
    {
        DrawTutorialPanel(
            "COMBAT TRAINING",
            "Press SPACE to attack",
            "3 / 3"
        );
        return;
    }

    DrawTutorialPanel(
        "TRAINING COMPLETE",
        "Defeat the Zombie Student",
        "READY"
    );
}

static void DrawCombatObjective(
    const Level1 *level
)
{
    if (level->messageTimer <= 0.0f)
    {
        return;
    }

    float alpha = ClampFloat(
        level->messageTimer / 0.45f,
        0.0f,
        1.0f
    );

    const int panelWidth = 430;
    const int panelHeight = 68;
    const int panelX =
        SCREEN_WIDTH / 2 - panelWidth / 2;
    const int panelY = 90;

    DrawRectangle(
        panelX,
        panelY,
        panelWidth,
        panelHeight,
        Fade(BLACK, 0.78f * alpha)
    );

    DrawRectangleLinesEx(
        (Rectangle){
            (float)panelX,
            (float)panelY,
            (float)panelWidth,
            (float)panelHeight
        },
        2.0f,
        Fade(RED, 0.90f * alpha)
    );

    DrawCenteredText(
        "LEVEL 1 OBJECTIVE",
        panelY + 10,
        16,
        Fade(RED, alpha)
    );

    DrawCenteredText(
        "DEFEAT THE ZOMBIE STUDENT",
        panelY + 34,
        21,
        Fade(RAYWHITE, alpha)
    );
}

static void DrawPostFightDialogue(
    const Level1 *level
)
{
    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade(BLACK, 0.16f)
    );

    Rectangle box = {
        95.0f,
        405.0f,
        810.0f,
        135.0f
    };

    DrawRectangleRounded(
        box,
        0.06f,
        10,
        Fade(BLACK, 0.93f)
    );

    DrawRectangleLinesEx(
        box,
        3.0f,
        Fade(RED, 0.90f)
    );

    DrawRectangle(
        (int)box.x,
        (int)box.y,
        8,
        (int)box.height,
        RED
    );

    DrawText(
        "PROFESSOR REAZ",
        (int)box.x + 28,
        (int)box.y + 17,
        20,
        RED
    );

    DrawText(
        POST_FIGHT_DIALOGUE[
            level->dialogueIndex
        ],
        (int)box.x + 28,
        (int)box.y + 54,
        22,
        RAYWHITE
    );

    if (level->dialogueFinished)
    {
        DrawText(
            "Dialogue complete — Story Clue 01 will be recovered next.",
            (int)box.x + 28,
            (int)box.y + 94,
            17,
            ORANGE
        );
    }
    else
    {
        DrawText(
            "Press ENTER to continue",
            (int)box.x + 565,
            (int)box.y + 100,
            16,
            LIGHTGRAY
        );
    }
}

void DrawLevel1Overlay(
    const Level1 *level,
    Texture2D campusDestroyed,
    Texture2D missionPaperScene,
    Texture2D redBoxLake
)
{
    if (level == 0)
    {
        return;
    }

    if (level->state == LEVEL1_TUTORIAL)
    {
        DrawTutorialOverlay(level);
        return;
    }

    if (level->state == LEVEL1_COMBAT)
    {
        DrawCombatObjective(level);
        return;
    }

    if (level->state == LEVEL1_POST_FIGHT)
    {
        DrawPostFightDialogue(level);
        return;
    }

    if (level->state == LEVEL1_OPENING)
    {
        float duration =
            GetSceneDuration(
                level->openingTextIndex
            );

        float alpha = GetFadeAlpha(
            level->stateTimer,
            duration
        );

        /*
           Keep the opening fully opaque.
           Without this black base, the gameplay Main Gate background
           becomes visible for a moment while a scene alpha is near zero.
        */
        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            BLACK
        );

        /*
           The paper scene uses a longer 1.5-second fade-out.
           Other opening scenes keep the normal fade duration.
        */
        if (level->openingTextIndex == 2)
        {
            const float paperFadeOutDuration = 1.50f;

            if (
                level->stateTimer >
                duration - paperFadeOutDuration
            )
            {
                alpha =
                    (duration - level->stateTimer) /
                    paperFadeOutDuration;

                alpha = ClampFloat(
                    alpha,
                    0.0f,
                    1.0f
                );
            }
        }

        switch (level->openingTextIndex)
        {
            case 0:
                DrawTitleScene(level, alpha);
                break;

            case 1:
                DrawCampusScene(
                    level,
                    campusDestroyed,
                    alpha
                );
                break;

            case 2:
                DrawPaperScene(
                    level,
                    missionPaperScene,
                    alpha
                );
                break;

            default:
                DrawWarningScene(
                    level,
                    redBoxLake,
                    alpha
                );
                break;
        }

        DrawText(
            "ENTER: next    SPACE: skip opening",
            SCREEN_WIDTH - 310,
            SCREEN_HEIGHT - 34,
            15,
            Fade(LIGHTGRAY, 0.74f)
        );

        return;
    }

    if (level->state == LEVEL1_AREA_INTRO)
    {
        const float fadeInDuration = 0.80f;
        const float fadeOutDuration = 0.90f;

        float titleAlpha = 1.0f;

        if (level->stateTimer < fadeInDuration)
        {
            titleAlpha = SmoothStep01(
                level->stateTimer /
                fadeInDuration
            );
        }
        else if (
            level->stateTimer >
            AREA_INTRO_DURATION - fadeOutDuration
        )
        {
            titleAlpha = SmoothStep01(
                (
                    AREA_INTRO_DURATION -
                    level->stateTimer
                ) /
                fadeOutDuration
            );
        }

        titleAlpha = ClampFloat(
            titleAlpha,
            0.0f,
            1.0f
        );

        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            Fade(BLACK, 0.42f * titleAlpha)
        );

        DrawLetterboxBars();

        DrawCenteredText(
            "ENTERING...",
            192,
            21,
            Fade(LIGHTGRAY, titleAlpha)
        );

        DrawCenteredText(
            "IUT MAIN GATE",
            235,
            42,
            Fade(RED, titleAlpha)
        );

        DrawRectangle(
            SCREEN_WIDTH / 2 - 120,
            293,
            240,
            3,
            Fade(RED, 0.86f * titleAlpha)
        );

        DrawCenteredText(
            "THE BEGINNING OF SURVIVAL",
            316,
            23,
            Fade(RAYWHITE, titleAlpha)
        );

        DrawCenteredText(
            "CAMPUS STABILITY: 10% - CRITICAL",
            382,
            18,
            Fade(ORANGE, titleAlpha)
        );

        DrawText(
            "Press ENTER to continue",
            SCREEN_WIDTH - 230,
            SCREEN_HEIGHT - 34,
            16,
            Fade(LIGHTGRAY, 0.72f * titleAlpha)
        );
    }
}