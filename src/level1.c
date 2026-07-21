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
#define AREA_INTRO_DURATION 3.40f
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

    return level->state == LEVEL1_TUTORIAL ||
           level->state == LEVEL1_COMBAT;
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
    DrawLetterboxBars();

    DrawCenteredText(
        "THE FALL OF IUT",
        78,
        31,
        Fade(RAYWHITE, alpha)
    );

    DrawCenteredText(
        "Campus systems collapsed without warning.",
        446,
        22,
        Fade(RAYWHITE, alpha)
    );

    DrawCenteredText(
        "Every department fell to corruption.",
        478,
        20,
        Fade(LIGHTGRAY, alpha)
    );

    DrawCenteredText(
        "Everyone disappeared. Only one professor remained.",
        510,
        20,
        Fade(LIGHTGRAY, alpha)
    );
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
    const int titleSize = 14;
    const int subtitleSize = 15;
    const int bodySize = 14;
    const int lineGap = 29;

    Color ink = Fade((Color){48, 29, 18, 255}, alpha);
    Color darkRed = Fade((Color){119, 25, 20, 255}, alpha);
    Color rule = Fade((Color){94, 55, 35, 255}, 0.84f * alpha);

    DrawTextCenteredInPaper(
        MISSION_TITLE,
        paperCenter-24,
        titleY+8,
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

    DrawTextureCover(
        missionPaperScene,
        1.0f + progress * 0.018f,
        Fade(WHITE, alpha)
    );

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade(BLACK, 0.06f)
    );

    DrawMissionText(alpha);
    DrawLetterboxBars();

    DrawCenteredText(
        "EMERGENCY DIRECTIVE RECEIVED",
        67,
        19,
        Fade(RED, alpha)
    );
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
        float alpha = GetFadeAlpha(
            level->stateTimer,
            AREA_INTRO_DURATION
        );

        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            Fade(BLACK, 0.60f * alpha)
        );

        DrawLetterboxBars();

        DrawCenteredText(
            "ENTERING...",
            192,
            21,
            Fade(LIGHTGRAY, alpha)
        );

        DrawCenteredText(
            "IUT MAIN GATE",
            235,
            42,
            Fade(RED, alpha)
        );

        DrawRectangle(
            SCREEN_WIDTH / 2 - 120,
            293,
            240,
            3,
            Fade(RED, 0.86f * alpha)
        );

        DrawCenteredText(
            "THE BEGINNING OF SURVIVAL",
            316,
            23,
            Fade(RAYWHITE, alpha)
        );

        DrawCenteredText(
            "CAMPUS STABILITY: 10% - CRITICAL",
            382,
            18,
            Fade(ORANGE, alpha)
        );

        DrawText(
            "Press ENTER to continue",
            SCREEN_WIDTH - 230,
            SCREEN_HEIGHT - 34,
            16,
            Fade(LIGHTGRAY, 0.72f * alpha)
        );
    }
}