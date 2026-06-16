@echo off
REM Build script for AT-USB LoRa Dongle - Hardware Variant Builder
REM Usage: build_hw_variant.bat [variant] [board_rev] [build_type]
REM   variant:   868_XTAL, 868_TCXO, 915_XTAL, 915_TCXO  (default: 915_TCXO)
REM   board_rev: v1, v2                                    (default: v1)
REM   build_type: Debug, Release                           (default: Release)
REM
REM Example: build_hw_variant.bat 868_XTAL v2 Release

setlocal EnableDelayedExpansion

set HW_VARIANT=%1
set BOARD_REV=%2
set BUILD_TYPE=%3

if "%HW_VARIANT%"=="" set HW_VARIANT=915_TCXO
if "%BOARD_REV%"==""  set BOARD_REV=v1
if "%BUILD_TYPE%"=""  set BUILD_TYPE=Release

REM Map variant to preset prefix
if "%HW_VARIANT%"=="868_XTAL" set PRESET_HW=868-xtal
if "%HW_VARIANT%"=="868_TCXO" set PRESET_HW=868-tcxo
if "%HW_VARIANT%"=="915_XTAL" set PRESET_HW=915-xtal
if "%HW_VARIANT%"=="915_TCXO" set PRESET_HW=915-tcxo

if not defined PRESET_HW (
    echo ERROR: Unknown variant "%HW_VARIANT%"
    exit /b 1
)

REM Map board rev
if /I "%BOARD_REV%"=="v1" set PRESET_REV=v1
if /I "%BOARD_REV%"=="v2" set PRESET_REV=v2

if not defined PRESET_REV (
    echo ERROR: Unknown board rev "%BOARD_REV%"
    exit /b 1
)

REM Map build type
if /I "%BUILD_TYPE%"=="Debug"   set PRESET_BUILD=debug
if /I "%BUILD_TYPE%"=="Release" set PRESET_BUILD=release

if not defined PRESET_BUILD (
    echo ERROR: Unknown build type "%BUILD_TYPE%"
    exit /b 1
)

set PRESET=%PRESET_HW%-%PRESET_REV%-%PRESET_BUILD%

echo ========================================
echo HW Variant:  %HW_VARIANT%
echo Board Rev:   %BOARD_REV%
echo Build Type:  %BUILD_TYPE%
echo CMake preset: %PRESET%
echo ========================================
echo.

cd /d "%~dp0"

echo Configuring...
cmake --preset %PRESET%
if errorlevel 1 (
    echo ERROR: Configuration failed for preset %PRESET%
    exit /b 1
)

echo.
echo Building...
cmake --build --preset %PRESET% -j
if errorlevel 1 (
    echo ERROR: Build failed for preset %PRESET%
    exit /b 1
)

echo.
echo ========================================
echo Build OK: build\%PRESET%\CmakeTest.elf
echo ========================================

endlocal
