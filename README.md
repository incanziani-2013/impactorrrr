# IMPACTOR 1.0 PRO

Transient shaper VST3 (JUCE 8.0.6 + CMake) para FL Studio en Windows.

**Estado actual: ETAPA 1 completa** (estructura + motor DSP + procesador + editor básico).
Las etapas siguientes (UI profesional, mascota I.M.P., backend Groq) se agregan sobre esta base.

## Qué hace (ya funciona)

| Parámetro | ID (no cambiar) | Rango | Por defecto | Qué hace |
|---|---|---|---|---|
| Attack | `attack` | 0–100 | 50 | 50 = neutro. Más de 50 realza el golpe (hasta +12 dB), menos de 50 lo suaviza |
| Sustain | `sustain` | 0–100 | 50 | 50 = neutro. Más de 50 alarga la cola, menos de 50 la acorta |
| Body | `body` | 0–100 | 0 | Realce de graves (shelf a 110 Hz, hasta +9 dB) |
| Punch | `punch` | 0–100 | 0 | Suma grave (<150 Hz) solo durante el golpe |
| Clip | `clip` | 0–100 | 0 | Saturación suave; limita los picos |
| Mix | `mix` | 0–100 % | 100 | Dry/wet en paralelo |
| Input Gain | `inputGain` | -24…+24 dB | 0 | Ganancia de entrada |
| Output Gain | `outputGain` | -24…+24 dB | 0 | Ganancia de salida |
| Bypass | `bypass` | on/off | off | Pasa la señal original (con crossfade, sin clicks) |

Todos son automatizables en FL Studio y se guardan con el proyecto.
Con ATTACK alto la salida puede pasar de 0 dBFS: bajá OUTPUT o subí CLIP.

## Requisitos (Windows 10/11)

1. **Visual Studio 2022** o **Build Tools para Visual Studio 2022**, con la carga de trabajo *Desarrollo para el escritorio con C++*.
2. **CMake** (https://cmake.org/download/, marcá "Add CMake to the system PATH").
3. **Git** (opcional, solo si querés que `setup_juce.bat` descargue JUCE por vos).

## Compilar

1. Abrí la carpeta `IMPACTOR` en VS Code.
2. Doble clic en `build.bat` (o en la terminal de VS Code: `.\build.bat`).
3. La primera vez busca JUCE; si no lo encuentra, te ofrece descargarlo.
   La ruta queda guardada en `juce_path.txt`. También podés usar la variable de entorno `JUCE_DIR`
   o pasar `-DIMPACTOR_JUCE_DIR=C:/ruta/a/JUCE` a CMake.
4. Resultado: `build\IMPACTOR_artefacts\Release\VST3\IMPACTOR.vst3`

## Instalar en FL Studio

1. Clic derecho en `instalar_vst3.bat` → **Ejecutar como administrador**
   (copia el plugin a `C:\Program Files\Common Files\VST3`).
2. FL Studio → *Options → Manage plugins → Find installed plugins (Start scan)*.
3. Buscá **IMPACTOR** en la lista y agregalo a un canal del Mixer.

## Tests

```
cmake --build build --config Release --target impactor_dsp_test
build\Release\impactor_dsp_test.exe
```

Prueban el motor DSP (neutro exacto, attack, sustain, body, punch, clip, mix, bypass, NaN/Inf, silencio,
cambios bruscos de parámetros, estéreo). También existe `impactor_processor_test` (parámetros, guardado de estado, audio).

## Estructura

```
IMPACTOR/
  CMakeLists.txt  build.bat  setup_juce.bat  instalar_vst3.bat
  .gitignore  .env.example  README.md
  Source/
    PluginProcessor.h/.cpp    parámetros (APVTS), estado, audio
    PluginEditor.h/.cpp       editor de la Etapa 1
    DSP/
      TransientProcessor.h/.cpp   motor: detector de transitorios, body, punch, mix, bypass
      SoftClipper.h/.cpp          saturación suave
  tests/
    dsp_test.cpp  processor_test.cpp
```

## Seguridad

La clave de Groq (`GROQ_API_KEY`) vivirá **solo** en el servidor de `Server/` (Etapa 4), leída desde `.env`.
El `.env` está en `.gitignore`: nunca va al plugin ni a GitHub.

## Qué está verificado y qué no

- ✔ El motor DSP compila sin advertencias y pasa todos sus tests.
- ✔ El plugin completo (VST3) compila y enlaza con JUCE 8.0.6, y el procesador pasa sus tests
  (parámetros, guardado/restauración de estado, layouts mono/estéreo, audio) — **verificado en Linux**.
- ✘ **Todavía no se compiló con Visual Studio en Windows ni se cargó en FL Studio.**
  Eso lo vas a probar vos con `build.bat`; si algo falla, mandame el mensaje de error.
