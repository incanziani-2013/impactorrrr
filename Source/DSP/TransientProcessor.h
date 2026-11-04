#pragma once

#include "SoftClipper.h"

namespace impactor
{
/**
 * Motor DSP de IMPACTOR (transient shaper).
 *
 * - C++ puro, sin JUCE, sin memoria dinamica, sin locks: seguro para el hilo de audio.
 * - Soporta 1 o 2 canales (deteccion enlazada para no mover la imagen estereo).
 * - Sin latencia.
 *
 * Cadena por muestra:
 *   entrada -> input gain -> [ATTACK / SUSTAIN] -> [BODY] -> [PUNCH] -> [CLIP]
 *           -> MIX (dry/wet) -> output gain -> BYPASS (crossfade)
 *
 * Escalas de parametros (las mismas que usan los presets y la IA):
 *   attack, sustain : 0..100, 50 = neutro. <50 reduce, >50 realza.
 *   body, punch, clip : 0..100, 0 = apagado.
 *   mix             : 0..100 (%), 100 = todo procesado.
 *   inputGainDb, outputGainDb : -24..+24 dB.
 */
class TransientProcessor
{
public:
    static constexpr int kMaxChannels = 2;

    struct Params
    {
        float attack       = 50.0f;
        float sustain      = 50.0f;
        float body         = 0.0f;
        float punch        = 0.0f;
        float clip         = 0.0f;
        float mix          = 100.0f;
        float inputGainDb  = 0.0f;
        float outputGainDb = 0.0f;
        bool  bypass       = false;
    };

    /** Llamar fuera del hilo de audio (o al menos antes de process). */
    void prepare (double sampleRate) noexcept;

    /** Limpia estados internos (envolventes, filtros). */
    void reset() noexcept;

    /** Se puede llamar al inicio de cada bloque. Los cambios se suavizan solos. */
    void setParams (const Params& p) noexcept;

    /** Procesa en el lugar. Canales mas alla de kMaxChannels se dejan intactos. */
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

private:
    struct BiquadCoefs { double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0; };
    struct BiquadState { double z1 = 0.0, z2 = 0.0; };

    struct Smooth
    {
        float cur = 0.0f;
        float target = 0.0f;
    };

    void updateShelf (float bodyDb) noexcept;

    double sampleRate = 44100.0;

    // coeficientes de un polo
    float fastAtk = 0.0f, fastRel = 0.0f, slowAtk = 0.0f, slowRel = 0.0f;
    float gainCoef = 0.0f, smoothCoef = 0.0f;

    // parametros suavizados
    Smooth attack, sustain, body, punch, clip, mix, inGain, outGain, bypassMix;
    bool needSnap = true;

    // detector
    float envFast = 0.0f, envSlow = 0.0f;
    float gainDbSmooth = 0.0f, transientSmooth = 0.0f;

    // filtros
    BiquadCoefs shelfCoefs, lowpassCoefs;
    BiquadState shelfState[kMaxChannels], lowpassState[kMaxChannels];
    float lastBodyDb = 0.0f;

    SoftClipper::Settings clipSettings;
    int subCounter = 0;
};
} // namespace impactor
