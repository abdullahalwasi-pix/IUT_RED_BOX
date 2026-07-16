#include "game.h"
#include "config.h"
#include <math.h>

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

static void LoadGameAudio(GameAudio *audio)
{
    *audio = (GameAudio){0};

    audio->swordSwing = LoadSound(
        "../assets/audio/sword_swing.wav"
    );

    audio->swordHit = LoadSound(
        "../assets/audio/sword_hit.wav"
    );

    audio->playerHit = LoadSound(
        "../assets/audio/player_hit.wav"
    );

    audio->enemyTelegraph = LoadSound(
        "../assets/audio/enemy_telegraph.wav"
    );

    audio->enemyPush = LoadSound(
        "../assets/audio/enemy_push.wav"
    );

    audio->victory = LoadSound(
        "../assets/audio/victory.wav"
    );

    audio->defeat = LoadSound(
        "../assets/audio/defeat.wav"
    );

    audio->level1Intro = LoadSound(
        "../assets/audio/level1_intro.wav"
    );

    audio->level1Ambience = LoadMusicStream(
        "../assets/audio/level1_ambience.wav"
    );

    audio->swordSwingReady =
        IsSoundValid(audio->swordSwing);

    audio->swordHitReady =
        IsSoundValid(audio->swordHit);

    audio->playerHitReady =
        IsSoundValid(audio->playerHit);

    audio->enemyTelegraphReady =
        IsSoundValid(audio->enemyTelegraph);

    audio->enemyPushReady =
        IsSoundValid(audio->enemyPush);

    audio->victoryReady =
        IsSoundValid(audio->victory);

    audio->defeatReady =
        IsSoundValid(audio->defeat);

    audio->level1IntroReady =
        IsSoundValid(audio->level1Intro);

    audio->level1AmbienceReady =
        IsMusicValid(audio->level1Ambience);

    if (audio->swordSwingReady)
    {
        SetSoundVolume(
            audio->swordSwing,
            0.42f
        );
    }

    if (audio->swordHitReady)
    {
        SetSoundVolume(
            audio->swordHit,
            0.62f
        );
    }

    if (audio->playerHitReady)
    {
        SetSoundVolume(
            audio->playerHit,
            0.48f
        );
    }

    if (audio->enemyTelegraphReady)
    {
        SetSoundVolume(
            audio->enemyTelegraph,
            0.34f
        );
    }

    if (audio->enemyPushReady)
    {
        SetSoundVolume(
            audio->enemyPush,
            0.46f
        );
    }

    if (audio->victoryReady)
    {
        SetSoundVolume(
            audio->victory,
            0.50f
        );
    }

    if (audio->defeatReady)
    {
        SetSoundVolume(
            audio->defeat,
            0.50f
        );
    }

    if (audio->level1IntroReady)
    {
        SetSoundVolume(
            audio->level1Intro,
            0.52f
        );
    }

    if (audio->level1AmbienceReady)
    {
        SetMusicVolume(
            audio->level1Ambience,
            0.20f
        );

        audio->level1Ambience.looping = true;
    }
}

static void UnloadGameAudio(GameAudio *audio)
{
    if (audio->swordSwingReady)
    {
        UnloadSound(audio->swordSwing);
    }

    if (audio->swordHitReady)
    {
        UnloadSound(audio->swordHit);
    }

    if (audio->playerHitReady)
    {
        UnloadSound(audio->playerHit);
    }

    if (audio->enemyTelegraphReady)
    {
        UnloadSound(audio->enemyTelegraph);
    }

    if (audio->enemyPushReady)
    {
        UnloadSound(audio->enemyPush);
    }

    if (audio->victoryReady)
    {
        UnloadSound(audio->victory);
    }

    if (audio->defeatReady)
    {
        UnloadSound(audio->defeat);
    }

    if (audio->level1IntroReady)
    {
        UnloadSound(audio->level1Intro);
    }

    if (audio->level1AmbienceReady)
    {
        UnloadMusicStream(
            audio->level1Ambience
        );
    }

    *audio = (GameAudio){0};
}

static bool IsPlayerAboveEnemy(
    const Player *player,
    const ZombieEnemy *enemy
)
{
    float playerBottomY =
        player->body.y +
        player->body.height;

    float enemyTopY =
        enemy->body.y;

    return playerBottomY <
           enemyTopY + 50.0f;
}

static void ResolvePlayerAgainstEnemy(
    Game *game
)
{
    Player *player = &game->player;
    ZombieEnemy *enemy = &game->enemy;

    bool playerAboveEnemy =
        IsPlayerAboveEnemy(
            player,
            enemy
        );

    if (
        game->state != GAME_PLAYING ||
        !enemy->active ||
        enemy->state == ENEMY_DYING ||
        player->isDying ||
        player->deathFinished ||
        playerAboveEnemy ||
        !CheckCollisionRecs(
            player->body,
            enemy->body
        )
    )
    {
        return;
    }

    float previousBodyX =
        player->previousX +
        player->referenceDrawWidth *
        0.30f;

    float previousBodyCenterX =
        previousBodyX +
        player->body.width *
        0.5f;

    float enemyCenterX =
        enemy->body.x +
        enemy->body.width *
        0.5f;

    if (
        previousBodyCenterX <=
        enemyCenterX
    )
    {
        player->x =
            enemy->body.x -
            player->body.width -
            player->referenceDrawWidth *
            0.30f;
    }
    else
    {
        player->x =
            enemy->body.x +
            enemy->body.width -
            player->referenceDrawWidth *
            0.30f;
    }

    player->x = ClampFloat(
        player->x,
        0.0f,
        (float)SCREEN_WIDTH -
            player->referenceDrawWidth
    );

    RefreshPlayerGeometry(
        player,
        &game->playerAssets
    );
}

static void ResolveEnemyAgainstPlayer(
    Game *game
)
{
    Player *player = &game->player;
    ZombieEnemy *enemy = &game->enemy;

    bool playerAboveEnemy =
        IsPlayerAboveEnemy(
            player,
            enemy
        );

    if (
        game->state != GAME_PLAYING ||
        !enemy->active ||
        enemy->state == ENEMY_DYING ||
        player->isDying ||
        player->deathFinished ||
        playerAboveEnemy ||
        !CheckCollisionRecs(
            player->body,
            enemy->body
        )
    )
    {
        return;
    }

    float playerCenterX =
        player->body.x +
        player->body.width *
        0.5f;

    float enemyCenterX =
        enemy->body.x +
        enemy->body.width *
        0.5f;

    if (enemyCenterX >= playerCenterX)
    {
        enemy->body.x =
            player->body.x +
            player->body.width;
    }
    else
    {
        enemy->body.x =
            player->body.x -
            enemy->body.width;
    }

    enemy->body.x = ClampFloat(
        enemy->body.x,
        0.0f,
        (float)SCREEN_WIDTH -
            enemy->body.width
    );
}

static void UpdateCombat(Game *game, float dt)
{
    Player *player = &game->player;
    ZombieEnemy *enemy = &game->enemy;

    bool playerAboveEnemy =
        IsPlayerAboveEnemy(
            player,
            enemy
        );

    UpdateZombieEnemy(
        enemy,
        player->body,
        playerAboveEnemy,
        player->health > 0,
        game->state == GAME_PLAYING,
        dt,
        game->audio.enemyTelegraph,
        game->audio.enemyTelegraphReady
    );

    if (
        enemy->attackWindowOpened &&
        game->state == GAME_PLAYING
    )
    {
        if (game->audio.enemyPushReady)
        {
            PlaySound(
                game->audio.enemyPush
            );
        }

        if (
            !playerAboveEnemy &&
            CheckCollisionRecs(
                enemy->attackBox,
                player->body
            ) &&
            player->invulnerabilityTimer <=
                0.0f &&
            player->health > 0
        )
        {
            ApplyDamageToPlayer(
                player,
                &game->playerAssets,
                enemy->damage
            );

            game->cameraShakeTimer =
                0.10f;

            game->impactFlashTimer =
                0.06f;

            if (game->audio.playerHitReady)
            {
                PlaySound(
                    game->audio.playerHit
                );
            }
        }

        MarkZombieAttackResolved(enemy);
    }

    if (
        game->state == GAME_PLAYING &&
        player->attackActive &&
        !player->enemyHitThisAttack &&
        enemy->state != ENEMY_DYING &&
        enemy->health > 0
    )
    {
        float horizontalGap =
            GetZombieHorizontalGap(
                enemy,
                player->body
            );

        if (
            horizontalGap <=
                GetPlayerAttackGapLimit(
                    player
                ) &&
            CheckCollisionRecs(
                player->attackBox,
                enemy->body
            )
        )
        {
            float playerCenterX =
                player->body.x +
                player->body.width *
                0.5f;

            float enemyCenterX =
                enemy->body.x +
                enemy->body.width *
                0.5f;

            DamageZombie(
                enemy,
                GetPlayerAttackDamage(
                    player
                ),
                playerCenterX <
                    enemyCenterX
            );

            player->enemyHitThisAttack = true;
            game->successfulHits++;

            game->cameraShakeTimer =
                0.10f;

            game->impactFlashTimer =
                player->specialAttack
                    ? 0.10f
                    : 0.06f;

            if (game->audio.swordHitReady)
            {
                SetSoundPitch(
                    game->audio.swordHit,
                    player->specialAttack
                        ? 0.90f
                        : 1.00f
                );

                PlaySound(
                    game->audio.swordHit
                );
            }
        }
    }
}

static void UpdateGameResult(Game *game, float dt)
{
    if (game->state == GAME_PLAYING)
    {
        if (game->player.deathFinished)
        {
            game->state = GAME_DEFEAT;
            game->resultTimer = 0.0f;

            LockPlayerDeathFinalPose(
                &game->player
            );

            if (
                game->enemy.active &&
                game->enemy.state !=
                    ENEMY_DYING
            )
            {
                ForceZombieVictory(
                    &game->enemy
                );
            }

            if (game->audio.level1AmbienceReady)
            {
                StopMusicStream(
                    game->audio.level1Ambience
                );
            }

            if (
                !game->defeatSoundPlayed &&
                game->audio.defeatReady
            )
            {
                PlaySound(
                    game->audio.defeat
                );

                game->defeatSoundPlayed = true;
            }
        }
        else if (
            game->enemy.deathFinished &&
            game->player.health > 0
        )
        {
            ForceZombieCorpse(
                &game->enemy
            );

            game->state = GAME_VICTORY;
            game->resultTimer = 0.0f;

            StartPlayerVictory(
                &game->player
            );

            if (game->audio.level1AmbienceReady)
            {
                StopMusicStream(
                    game->audio.level1Ambience
                );
            }

            if (
                !game->victorySoundPlayed &&
                game->audio.victoryReady
            )
            {
                PlaySound(
                    game->audio.victory
                );

                game->victorySoundPlayed = true;
            }
        }
    }
    else
    {
        game->resultTimer += dt;
    }
}

static void DrawHud(const Game *game)
{
    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        112,
        Fade(BLACK, 0.72f)
    );

    DrawText(
        "IUT RED BOX",
        24,
        18,
        30,
        RED
    );

    DrawText(
        TextFormat(
            "LEVEL %d",
            CURRENT_LEVEL
        ),
        25,
        58,
        24,
        RAYWHITE
    );

    DrawText(
        "PROFESSOR",
        690,
        12,
        17,
        RAYWHITE
    );

    Rectangle playerBarBack = {
        690.0f,
        35.0f,
        270.0f,
        20.0f
    };

    Rectangle playerBarFill = {
        playerBarBack.x,
        playerBarBack.y,
        playerBarBack.width *
            (
                (float)game->player.health /
                (float)PLAYER_MAX_HEALTH
            ),
        playerBarBack.height
    };

    DrawRectangleRounded(
        playerBarBack,
        0.28f,
        8,
        DARKGRAY
    );

    DrawRectangleRounded(
        playerBarFill,
        0.28f,
        8,
        game->player.health > 30
            ? LIME
            : RED
    );

    DrawRectangleLinesEx(
        playerBarBack,
        2.0f,
        RAYWHITE
    );

    DrawText(
        TextFormat(
            "%d / %d",
            game->player.health,
            PLAYER_MAX_HEALTH
        ),
        786,
        37,
        17,
        BLACK
    );

    const char *comboText =
        game->player.specialAttack &&
        game->player.isAttacking
            ? "COMBO: SPECIAL ATTACK!"
            : TextFormat(
                "COMBO: %d / %d",
                game->player.comboAttackCount,
                COMBO_REQUIRED_ATTACKS
            );

    Rectangle comboBox = {
        690.0f,
        68.0f,
        270.0f,
        29.0f
    };

    Color comboAccent =
        game->player.specialAttack &&
        game->player.isAttacking
            ? GOLD
            : SKYBLUE;

    DrawRectangleRounded(
        comboBox,
        0.25f,
        8,
        Fade(BLACK, 0.82f)
    );

    DrawRectangleLinesEx(
        comboBox,
        2.0f,
        comboAccent
    );

    int comboTextX =
        game->player.specialAttack &&
        game->player.isAttacking
            ? 728
            : 772;

    DrawText(
        comboText,
        comboTextX,
        (int)(comboBox.y + 5.0f),
        17,
        comboAccent
    );

    if (
        game->player.specialAttack &&
        game->player.isAttacking &&
        game->player.attackActive
    )
    {
        DrawText(
            "THIRD STRIKE!",
            410,
            145,
            25,
            GOLD
        );
    }
}

static void DrawResultPanel(const Game *game)
{
    if (
        game->state == GAME_PLAYING ||
        game->resultTimer <
            RESULT_PANEL_DELAY
    )
    {
        return;
    }

    DrawRectangle(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        Fade(BLACK, 0.62f)
    );

    Rectangle resultPanel = {
        230.0f,
        185.0f,
        540.0f,
        250.0f
    };

    Color accent =
        game->state == GAME_VICTORY
            ? LIME
            : RED;

    DrawRectangleRounded(
        resultPanel,
        0.08f,
        12,
        Fade(BLACK, 0.92f)
    );

    DrawRectangleLinesEx(
        resultPanel,
        3.0f,
        accent
    );

    if (game->state == GAME_VICTORY)
    {
        DrawText(
            "LEVEL 1 COMPLETE",
            330,
            215,
            34,
            LIME
        );

        DrawText(
            "Zombie Student defeated - area secured",
            320,
            260,
            21,
            RAYWHITE
        );

        DrawText(
            TextFormat(
                "Successful hits: %d",
                game->successfulHits
            ),
            390,
            305,
            20,
            LIGHTGRAY
        );

        DrawText(
            TextFormat(
                "Health remaining: %d",
                game->player.health
            ),
            388,
            335,
            20,
            LIGHTGRAY
        );

        DrawText(
            "VICTORY",
            435,
            375,
            26,
            GOLD
        );
    }
    else
    {
        DrawText(
            "MISSION FAILED",
            357,
            215,
            34,
            RED
        );

        DrawText(
            "The Zombie Student defeated the player",
            317,
            260,
            21,
            RAYWHITE
        );

        DrawText(
            TextFormat(
                "Enemy health remaining: %d",
                game->enemy.health
            ),
            365,
            310,
            20,
            LIGHTGRAY
        );

        DrawText(
            "ENEMY WINS",
            408,
            355,
            27,
            ORANGE
        );
    }

    DrawText(
        "Press R to restart Level 1",
        365,
        402,
        18,
        GRAY
    );
}

bool InitGame(Game *game)
{
    if (game == 0)
    {
        return false;
    }

    *game = (Game){0};

    game->groundY = 530.0f;
    game->state = GAME_PLAYING;

    game->background = LoadTexture(
        "../assets/backgrounds/main_gate.png"
    );

    if (!IsTextureValid(game->background))
    {
        TraceLog(
            LOG_ERROR,
            "Background texture failed to load."
        );

        return false;
    }

    if (!LoadPlayerAssets(&game->playerAssets))
    {
        TraceLog(
            LOG_ERROR,
            "One or more player textures failed to load."
        );

        UnloadGame(game);
        return false;
    }

    if (
        !PlayerEndStateAssetLayoutIsValid(
            &game->playerAssets
        )
    )
    {
        TraceLog(
            LOG_ERROR,
            "Player hurt/death/victory sheets must use one shared equal-square-cell format."
        );

        UnloadGame(game);
        return false;
    }

    if (!LoadZombieAssets(&game->zombieAssets))
    {
        TraceLog(
            LOG_ERROR,
            "One or more zombie textures failed to load."
        );

        UnloadGame(game);
        return false;
    }

    if (
        !ZombieAssetLayoutIsValid(
            &game->zombieAssets
        )
    )
    {
        TraceLog(
            LOG_ERROR,
            "Zombie sheets must use equal square cells with one shared cell size."
        );

        UnloadGame(game);
        return false;
    }

    LoadGameAudio(&game->audio);

    game->target = LoadRenderTexture(
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    SetTextureFilter(
        game->target.texture,
        TEXTURE_FILTER_BILINEAR
    );

    game->backgroundSource = (Rectangle){
        0.0f,
        0.0f,
        (float)game->background.width,
        (float)game->background.height
    };

    game->backgroundDestination = (Rectangle){
        0.0f,
        0.0f,
        (float)SCREEN_WIDTH,
        (float)SCREEN_HEIGHT
    };

    InitPlayer(
        &game->player,
        &game->playerAssets,
        game->groundY
    );

    game->levelConfig =
        GetLevelEnemyConfig(
            CURRENT_LEVEL
        );

    float playerStandingDrawHeight =
        (float)game->playerAssets.idle.height *
        PLAYER_IDLE_SCALE;

    game->enemy = CreateZombieEnemy(
        760.0f,
        game->groundY,
        playerStandingDrawHeight,
        (float)game->zombieAssets.idle.height,
        game->levelConfig
    );

    game->camera = (Camera2D){0};
    game->camera.zoom = 1.0f;

    if (game->audio.level1IntroReady)
    {
        PlaySound(
            game->audio.level1Intro
        );
    }

    return true;
}

void RestartGame(Game *game)
{
    if (game == 0)
    {
        return;
    }

    ResetPlayer(
        &game->player,
        &game->playerAssets,
        game->groundY
    );

    float playerStandingDrawHeight =
        (float)game->playerAssets.idle.height *
        PLAYER_IDLE_SCALE;

    ResetZombieEnemy(
        &game->enemy,
        760.0f,
        game->groundY,
        playerStandingDrawHeight,
        (float)game->zombieAssets.idle.height,
        game->levelConfig
    );

    game->state = GAME_PLAYING;
    game->resultTimer = 0.0f;
    game->levelIntroTimer = 0.0f;
    game->cameraShakeTimer = 0.0f;
    game->impactFlashTimer = 0.0f;
    game->successfulHits = 0;
    game->ambienceStarted = false;
    game->victorySoundPlayed = false;
    game->defeatSoundPlayed = false;
    game->camera.offset = (Vector2){0.0f, 0.0f};

    if (game->audio.level1AmbienceReady)
    {
        StopMusicStream(
            game->audio.level1Ambience
        );
    }

    if (game->audio.level1IntroReady)
    {
        StopSound(
            game->audio.level1Intro
        );

        PlaySound(
            game->audio.level1Intro
        );
    }
}

void UpdateGame(Game *game)
{
    if (game == 0)
    {
        return;
    }

    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_F11))
    {
        ToggleBorderlessWindowed();
    }

    if (IsKeyPressed(KEY_F1))
    {
        game->showHitboxes =
            !game->showHitboxes;
    }

    if (IsKeyPressed(KEY_R))
    {
        RestartGame(game);
        return;
    }

    if (game->audio.level1AmbienceReady)
    {
        UpdateMusicStream(
            game->audio.level1Ambience
        );
    }

    if (
        game->state == GAME_PLAYING &&
        !game->ambienceStarted
    )
    {
        game->levelIntroTimer += dt;

        if (
            game->levelIntroTimer >= 1.35f &&
            game->audio.level1AmbienceReady
        )
        {
            PlayMusicStream(
                game->audio.level1Ambience
            );

            game->ambienceStarted = true;
        }
    }

    if (game->impactFlashTimer > 0.0f)
    {
        game->impactFlashTimer -= dt;
    }

    UpdatePlayer(
        &game->player,
        &game->playerAssets,
        dt,
        game->state == GAME_PLAYING,
        game->state == GAME_VICTORY,
        game->audio.swordSwing,
        game->audio.swordSwingReady
    );

    if (game->player.attackImpactEvent)
    {
        game->cameraShakeTimer =
            game->player.specialAttack
                ? 0.16f
                : 0.08f;

        game->impactFlashTimer =
            game->player.specialAttack
                ? 0.10f
                : 0.05f;
    }

    ResolvePlayerAgainstEnemy(game);
    UpdateCombat(game, dt);
    ResolveEnemyAgainstPlayer(game);
    UpdateGameResult(game, dt);

    if (game->cameraShakeTimer > 0.0f)
    {
        game->cameraShakeTimer -= dt;

        game->camera.offset.x =
            (float)GetRandomValue(-2, 2);

        game->camera.offset.y =
            (float)GetRandomValue(-2, 2);
    }
    else
    {
        game->camera.offset =
            (Vector2){0.0f, 0.0f};
    }
}

void DrawGame(Game *game)
{
    if (game == 0)
    {
        return;
    }

    BeginTextureMode(game->target);
    ClearBackground(BLACK);
    BeginMode2D(game->camera);

    DrawTexturePro(
        game->background,
        game->backgroundSource,
        game->backgroundDestination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        WHITE
    );

    DrawPlayer(
        &game->player,
        game->showHitboxes
    );

    DrawZombieEnemy(
        &game->enemy,
        &game->zombieAssets,
        game->showHitboxes
    );

    if (IsKeyDown(KEY_F2))
    {
        DrawLine(
            0,
            (int)game->groundY,
            SCREEN_WIDTH,
            (int)game->groundY,
            YELLOW
        );
    }

    EndMode2D();

    if (game->impactFlashTimer > 0.0f)
    {
        DrawRectangle(
            0,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            Fade(SKYBLUE, 0.08f)
        );
    }

    DrawHud(game);
    DrawResultPanel(game);

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    float realScreenWidth =
        (float)GetScreenWidth();

    float realScreenHeight =
        (float)GetScreenHeight();

    float screenScale = fminf(
        realScreenWidth /
            (float)SCREEN_WIDTH,
        realScreenHeight /
            (float)SCREEN_HEIGHT
    );

    float scaledWidth =
        (float)SCREEN_WIDTH *
        screenScale;

    float scaledHeight =
        (float)SCREEN_HEIGHT *
        screenScale;

    float screenOffsetX =
        (
            realScreenWidth -
            scaledWidth
        ) *
        0.5f;

    float screenOffsetY =
        (
            realScreenHeight -
            scaledHeight
        ) *
        0.5f;

    Rectangle targetSource = {
        0.0f,
        0.0f,
        (float)game->target.texture.width,
        -(float)game->target.texture.height
    };

    Rectangle targetDestination = {
        screenOffsetX,
        screenOffsetY,
        scaledWidth,
        scaledHeight
    };

    DrawTexturePro(
        game->target.texture,
        targetSource,
        targetDestination,
        (Vector2){0.0f, 0.0f},
        0.0f,
        WHITE
    );

    EndDrawing();
}

void UnloadGame(Game *game)
{
    if (game == 0)
    {
        return;
    }

    if (game->target.id != 0)
    {
        UnloadRenderTexture(
            game->target
        );
    }

    if (IsTextureValid(game->background))
    {
        UnloadTexture(
            game->background
        );
    }

    UnloadPlayerAssets(
        &game->playerAssets
    );

    UnloadZombieAssets(
        &game->zombieAssets
    );

    UnloadGameAudio(
        &game->audio
    );

    *game = (Game){0};
}
