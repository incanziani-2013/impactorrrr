@echo off
REM ============================================================
REM  IMPACTOR - copia el .vst3 compilado a la carpeta VST3 del sistema
REM  Necesita permisos de administrador: clic derecho > "Ejecutar como administrador"
REM ============================================================
setlocal
cd /d "%~dp0"

set "SRC=%~dp0build\IMPACTOR_artefacts\Release\VST3\IMPACTOR.vst3"
set "DST=C:\Program Files\Common Files\VST3"

if not exist "%SRC%" (
    echo [ERROR] No encuentro el plugin compilado. Ejecuta build.bat primero.
    pause
    exit /b 1
)

net session >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Falta permiso de administrador.
    echo         Clic derecho sobre instalar_vst3.bat y elige "Ejecutar como administrador".
    pause
    exit /b 1
)

if not exist "%DST%" mkdir "%DST%"

REM El .vst3 de JUCE es una carpeta (bundle) en Windows: se copia entera.
xcopy "%SRC%" "%DST%\IMPACTOR.vst3\" /E /I /Y /Q
if errorlevel 1 (
    echo [ERROR] No se pudo copiar. Cierra FL Studio y reintenta.
    pause
    exit /b 1
)

echo.
echo Instalado en "%DST%\IMPACTOR.vst3"
echo En FL Studio: Options ^> Manage plugins ^> Find installed plugins ^(Start scan^).
pause
endlocal
