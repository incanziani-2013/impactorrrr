@echo off
REM ============================================================
REM  IMPACTOR - compila el plugin VST3 (Release)
REM  Doble clic, o desde la terminal de VS Code:  .\build.bat
REM ============================================================
setlocal
cd /d "%~dp0"

echo.
echo === IMPACTOR 1.0 PRO - compilacion ===
echo.

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] No encuentro CMake. Instalalo desde https://cmake.org/download/
    echo         ^(marca "Add CMake to the system PATH"^) y vuelve a abrir la terminal.
    pause
    exit /b 1
)

call setup_juce.bat
if errorlevel 1 (
    pause
    exit /b 1
)
set /p JUCE_PATH=<"juce_path.txt"

echo.
echo [1/2] Configurando proyecto...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DIMPACTOR_JUCE_DIR="%JUCE_PATH%"
if errorlevel 1 (
    echo.
    echo [ERROR] Fallo la configuracion. Revisa que tengas instalado Visual Studio 2022
    echo         o "Build Tools para Visual Studio 2022" con la carga "Desarrollo para escritorio con C++".
    pause
    exit /b 1
)

echo.
echo [2/2] Compilando ^(la primera vez tarda varios minutos^)...
cmake --build build --config Release --parallel
if errorlevel 1 (
    echo.
    echo [ERROR] Fallo la compilacion. Copia el mensaje de error y mandamelo.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo  LISTO. Tu plugin esta en:
echo    %~dp0build\IMPACTOR_artefacts\Release\VST3\IMPACTOR.vst3
echo.
echo  Para instalarlo en FL Studio ejecuta instalar_vst3.bat
echo ============================================================
pause
endlocal
