@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "RAYLIB_PATH=C:\raylib\raylib"
set "GCC_PATH=C:\raylib\w64devkit\bin\gcc.exe"

cd /d "%PROJECT_DIR%src"

echo.
echo ========================================
echo      IUT RED BOX - BUILD AND RUN
echo ========================================
echo.

if exist main.exe del /F /Q main.exe

"%GCC_PATH%" -o main.exe main.c game.c player.c enemy.c level1.c "%RAYLIB_PATH%\src\raylib.rc.data" -s -static -O3 -std=c99 -Wall -Wshadow -Wunused-parameter -I"..\include" -I"%RAYLIB_PATH%\src" -DPLATFORM_DESKTOP -L. -lraylib -lopengl32 -lgdi32 -lwinmm -lshcore

if errorlevel 1 (
    echo.
    echo ========================================
    echo               BUILD FAILED
    echo ========================================
    echo Copy the full error and send it to ChatGPT.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo             BUILD SUCCESSFUL
echo ========================================
echo Starting the game...
echo.

main.exe

endlocal
