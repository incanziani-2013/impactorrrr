@echo off
REM ============================================================
REM  IMPACTOR - busca JUCE en tu PC (o lo descarga) y guarda la
REM  ruta en juce_path.txt. build.bat lo ejecuta solo.
REM ============================================================
setlocal EnableDelayedExpansion
cd /d "%~dp0"
set "FOUND="

if exist "juce_path.txt" (
    set /p CAND=<"juce_path.txt"
    call :check "!CAND!"
)
if not defined FOUND if defined JUCE_DIR call :check "%JUCE_DIR%"
if not defined FOUND call :check "%~dp0JUCE"
if not defined FOUND call :check "%~dp0third_party\JUCE"
if not defined FOUND call :check "%~dp0..\JUCE"
if not defined FOUND call :check "C:\JUCE"
if not defined FOUND call :check "%USERPROFILE%\JUCE"

if not defined FOUND (
    echo.
    echo No encontre JUCE en las carpetas habituales.
    where git >nul 2>nul
    if not errorlevel 1 (
        set /p ANS=Quieres descargar JUCE 8.0.6 ahora con git en la carpeta del proyecto? [S/N]:
        if /i "!ANS!"=="S" (
            git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git "%~dp0JUCE"
            call :check "%~dp0JUCE"
        )
    )
)

if not defined FOUND (
    set "MANUAL="
    set /p MANUAL=Escribe la ruta de tu carpeta JUCE ^(ENTER para cancelar^):
    if defined MANUAL call :check "!MANUAL!"
)

if not defined FOUND (
    echo.
    echo [ERROR] No se encontro JUCE. Descargalo de https://github.com/juce-framework/JUCE
    echo         y ejecuta este script otra vez.
    exit /b 1
)

> "juce_path.txt" echo !FOUND!
echo JUCE encontrado en: !FOUND!
endlocal & exit /b 0

:check
set "P=%~1"
if "%P%"=="" goto :eof
if exist "%P%\CMakeLists.txt" if exist "%P%\modules\juce_audio_processors\juce_audio_processors.h" set "FOUND=%P%"
goto :eof
