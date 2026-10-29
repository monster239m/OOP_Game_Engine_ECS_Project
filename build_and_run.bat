@echo off
REM ============================================================
REM  build_and_run.bat  -  One-click builder and runner for C++ ECS
REM ============================================================

echo ============================================================
echo   Compiling Extensible 2D Game Engine (ECS in C++)
echo ============================================================

g++ -std=c++14 -O2 -Iinclude src\main.cpp -o bin\GameEngineECS.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed! Check g++ compiler installation.
    pause
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Compilation successful! Output binary: bin\GameEngineECS.exe
echo.
echo ============================================================
echo   Running Game Engine Simulation Demo...
echo ============================================================
echo.

bin\GameEngineECS.exe

echo.
echo ============================================================
echo   Simulation finished!
echo ============================================================
pause
