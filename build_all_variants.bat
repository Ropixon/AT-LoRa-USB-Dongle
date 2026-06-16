@echo off
REM Build all hardware variants for AT-USB LoRa Dongle
REM Usage: build_all_variants.bat [release|all]
REM   release  - build only Release variants (default)
REM   all      - build both Debug and Release

setlocal EnableDelayedExpansion

cd /d "%~dp0"

set MODE=%1
if "%MODE%"=="" set MODE=release

echo ========================================
echo Building ALL Hardware Variants (mode: %MODE%)
echo ========================================
echo.

set VARIANTS=868_XTAL 868_TCXO 915_XTAL 915_TCXO
set BOARD_REVS=v1 v2

if /I "%MODE%"=="all" (
    set BUILD_TYPES=Debug Release
) else (
    set BUILD_TYPES=Release
)

set SUCCESS_COUNT=0
set FAIL_COUNT=0
set FAILED_BUILDS=

for %%V in (%VARIANTS%) do (
    for %%R in (%BOARD_REVS%) do (
        for %%B in (%BUILD_TYPES%) do (
            echo.
            echo ========================================
            echo Building: %%V - %%R - %%B
            echo ========================================
            call "%~dp0build_hw_variant.bat" %%V %%R %%B
            if errorlevel 1 (
                set /a FAIL_COUNT+=1
                set "FAILED_BUILDS=!FAILED_BUILDS! %%V-%%R-%%B"
            ) else (
                set /a SUCCESS_COUNT+=1
            )
        )
    )
)

echo.
echo ========================================
echo Build Summary
echo ========================================
echo Successful: %SUCCESS_COUNT%
echo Failed:     %FAIL_COUNT%
if not "!FAILED_BUILDS!"=="" (
    echo Failed builds:!FAILED_BUILDS!
)
echo ========================================

if %FAIL_COUNT% GTR 0 (
    exit /b 1
)

endlocal
