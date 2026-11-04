cmake_minimum_required(VERSION 3.22)

project(IMPACTOR VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Runtime de Visual C++ enlazado de forma estatica: el .vst3 no necesita instalar nada extra.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------------------
# Donde esta JUCE (configurable, en este orden):
#   1) -DIMPACTOR_JUCE_DIR=C:/ruta/a/JUCE   (lo que usa build.bat)
#   2) variable de entorno JUCE_DIR
#   3) carpetas habituales: ./JUCE, ./third_party/JUCE, ../JUCE, C:/JUCE
#   4) si no se encuentra y IMPACTOR_FETCH_JUCE=ON, se descarga JUCE 8.0.6 de GitHub
# ---------------------------------------------------------------------------
set(IMPACTOR_JUCE_DIR "" CACHE PATH "Carpeta raiz de JUCE (la que contiene CMakeLists.txt y modules/)")
option(IMPACTOR_FETCH_JUCE "Descargar JUCE 8.0.6 automaticamente si no se encuentra" ON)
option(IMPACTOR_COPY_AFTER_BUILD "Copiar el .vst3 a la carpeta VST3 del sistema tras compilar (requiere admin)" OFF)

set(_impactor_juce_candidates
    "${IMPACTOR_JUCE_DIR}"
    "$ENV{JUCE_DIR}"
    "${CMAKE_SOURCE_DIR}/JUCE"
    "${CMAKE_SOURCE_DIR}/third_party/JUCE"
    "${CMAKE_SOURCE_DIR}/../JUCE"
    "C:/JUCE")

set(_impactor_juce_found "")
foreach(_cand IN LISTS _impactor_juce_candidates)
    if(_cand AND EXISTS "${_cand}/CMakeLists.txt" AND EXISTS "${_cand}/modules/juce_audio_processors")
        set(_impactor_juce_found "${_cand}")
        break()
    endif()
endforeach()

if(_impactor_juce_found)
    message(STATUS "IMPACTOR: usando JUCE en ${_impactor_juce_found}")
    add_subdirectory("${_impactor_juce_found}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
elseif(IMPACTOR_FETCH_JUCE)
    message(STATUS "IMPACTOR: JUCE no encontrado, descargando JUCE 8.0.6 desde GitHub...")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
else()
    message(FATAL_ERROR
        "No se encontro JUCE. Ejecuta setup_juce.bat o pasa -DIMPACTOR_JUCE_DIR=C:/ruta/a/JUCE")
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(IMPACTOR
    COMPANY_NAME "IMPACTOR Audio"
    BUNDLE_ID com.impactoraudio.impactor
    PRODUCT_NAME "IMPACTOR"
    VERSION "1.0.0"
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Impa
    PLUGIN_CODE Imp1
    VST3_CATEGORIES Fx Dynamics
    COPY_PLUGIN_AFTER_BUILD ${IMPACTOR_COPY_AFTER_BUILD})

target_sources(IMPACTOR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/TransientProcessor.cpp
    Source/DSP/TransientProcessor.h
    Source/DSP/SoftClipper.cpp
    Source/DSP/SoftClipper.h)

target_include_directories(IMPACTOR PRIVATE Source)

target_compile_definitions(IMPACTOR PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(IMPACTOR
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Test del DSP (sin JUCE):  cmake --build build --target impactor_dsp_test
# ---------------------------------------------------------------------------
add_executable(impactor_dsp_test EXCLUDE_FROM_ALL
    tests/dsp_test.cpp
    Source/DSP/TransientProcessor.cpp
    Source/DSP/SoftClipper.cpp)
target_include_directories(impactor_dsp_test PRIVATE Source)

# ---------------------------------------------------------------------------
# Test del procesador JUCE (parametros, estado, audio):
#   cmake --build build --target impactor_processor_test
# ---------------------------------------------------------------------------
add_executable(impactor_processor_test EXCLUDE_FROM_ALL
    tests/processor_test.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/TransientProcessor.cpp
    Source/DSP/SoftClipper.cpp)
target_include_directories(impactor_processor_test PRIVATE Source)
target_compile_definitions(impactor_processor_test PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STANDALONE_APPLICATION=0)
target_link_libraries(impactor_processor_test PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags)
