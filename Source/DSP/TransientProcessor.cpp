#include "TransientProcessor.h"

#include <algorithm>
#include <cmath>

namespace impactor
{
namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr int    kSubBlock = 16;          // cada cuantas muestras se recalculan coeficientes
constexpr float  kMaxShapeDb = 12.0f;     // maximo realce/recorte por ATTACK/SUSTAIN
constexpr float  kBodyMaxDb = 9.0f;       // realce maximo del shelf de BODY
constexpr float  kPunchGain = 1.5f;       // cuanto grave se suma en PUNCH al 100 %

inline float sanitize (float v) noexcept
{
    if (! std::isfinite (v))
        return 0.0f;
    return std::min (16.0f, std::max (-16.0f, v));
}

inline float flush (float v) noexcept
{
    return std::fabs (v) < 1.0e-18f ? 0.0f : v;
}

inline double flushD (double v) noexcept
{
    return std::fabs (v) < 1.0e-30 ? 0.0 : v;
}

inline float onePoleCoef (double ms, double sr) noexcept
{
    return (float) std::exp (-1.0 / (ms * 0.001 * sr));
}

inline float dbToLin (float db) noexcept
{
    return std::exp (db * 0.115129255f);  // ln(10)/20
}

inline float clampf (float v, float lo, float hi) noexcept
{
    return std::min (hi, std::max (lo, v));
}

inline float follow (float env, float in, float atk, float rel) noexcept
{
    const float c = in > env ? atk : rel;
    return flush (in + (env - in) * c);
}

template <typename S>
inline void stepSmooth (S& s, float coef) noexcept
{
    s.cur = s.target + (s.cur - s.target) * coef;
    if (std::fabs (s.cur - s.target) < 1.0e-6f)
        s.cur = s.target;
}

template <typename C, typename S>
inline float runBiquad (const C& c, S& s, float x) noexcept
{
    const double in = (double) x;
    const double y = c.b0 * in + s.z1;
    s.z1 = flushD (c.b1 * in - c.a1 * y + s.z2);
    s.z2 = flushD (c.b2 * in - c.a2 * y);
    return (float) y;
}
} // namespace

void TransientProcessor::prepare (double sr) noexcept
{
    sampleRate = std::max (8000.0, sr);

    // Dos seguidores de envolvente: el rapido sigue el golpe, el lento sigue el cuerpo.
    // La diferencia entre ambos (en dB) indica si estamos en el ataque o en el sustain.
    fastAtk    = onePoleCoef (0.5, sampleRate);
    fastRel    = onePoleCoef (30.0, sampleRate);
    slowAtk    = onePoleCoef (20.0, sampleRate);
    slowRel    = onePoleCoef (120.0, sampleRate);
    gainCoef   = onePoleCoef (0.5, sampleRate);    // suaviza la ganancia dinamica (anti-click)
    smoothCoef = onePoleCoef (20.0, sampleRate);   // suaviza los parametros

    // Pasa-bajos fijo (150 Hz, Q 0.707) usado por PUNCH.
    {
        const double w0 = 2.0 * kPi * 150.0 / sampleRate;
        const double cosw = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * 0.70710678);
        const double a0 = 1.0 + alpha;
        lowpassCoefs.b0 = ((1.0 - cosw) * 0.5) / a0;
        lowpassCoefs.b1 = (1.0 - cosw) / a0;
        lowpassCoefs.b2 = ((1.0 - cosw) * 0.5) / a0;
        lowpassCoefs.a1 = (-2.0 * cosw) / a0;
        lowpassCoefs.a2 = (1.0 - alpha) / a0;
    }

    reset();
    needSnap = true;
}

void TransientProcessor::reset() noexcept
{
    envFast = envSlow = 0.0f;
    gainDbSmooth = transientSmooth = 0.0f;

    for (int c = 0; c < kMaxChannels; ++c)
    {
        shelfState[c] = {};
        lowpassState[c] = {};
    }

    shelfCoefs = {};
    lastBodyDb = 0.0f;
    clipSettings = SoftClipper::makeSettings (0.0f);
    subCounter = 0;
}

void TransientProcessor::setParams (const Params& p) noexcept
{
    attack.target    = clampf (p.attack, 0.0f, 100.0f);
    sustain.target   = clampf (p.sustain, 0.0f, 100.0f);
    body.target      = clampf (p.body, 0.0f, 100.0f);
    punch.target     = clampf (p.punch, 0.0f, 100.0f);
    clip.target      = clampf (p.clip, 0.0f, 100.0f);
    mix.target       = clampf (p.mix, 0.0f, 100.0f);
    inGain.target    = dbToLin (clampf (p.inputGainDb, -24.0f, 24.0f));
    outGain.target   = dbToLin (clampf (p.outputGainDb, -24.0f, 24.0f));
    bypassMix.target = p.bypass ? 1.0f : 0.0f;
}

void TransientProcessor::updateShelf (float bodyDb) noexcept
{
    // Low shelf RBJ (Audio EQ Cookbook), 110 Hz, S = 1.
    const double A = std::pow (10.0, (double) bodyDb / 40.0);
    const double w0 = 2.0 * kPi * 110.0 / sampleRate;
    const double cosw = std::cos (w0);
    const double sinw = std::sin (w0);
    const double alpha = sinw * 0.5 * std::sqrt (2.0);
    const double sqA2alpha = 2.0 * std::sqrt (A) * alpha;

    const double a0 = (A + 1.0) + (A - 1.0) * cosw + sqA2alpha;
    shelfCoefs.b0 = (A * ((A + 1.0) - (A - 1.0) * cosw + sqA2alpha)) / a0;
    shelfCoefs.b1 = (2.0 * A * ((A - 1.0) - (A + 1.0) * cosw)) / a0;
    shelfCoefs.b2 = (A * ((A + 1.0) - (A - 1.0) * cosw - sqA2alpha)) / a0;
    shelfCoefs.a1 = (-2.0 * ((A - 1.0) + (A + 1.0) * cosw)) / a0;
    shelfCoefs.a2 = ((A + 1.0) + (A - 1.0) * cosw - sqA2alpha) / a0;
}

void TransientProcessor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return;

    const int nch = std::min (numChannels, kMaxChannels);

    if (needSnap)
    {
        // Primer bloque tras prepare(): saltar directo al valor objetivo, sin rampa.
        for (Smooth* s : { &attack, &sustain, &body, &punch, &clip, &mix, &inGain, &outGain, &bypassMix })
            s->cur = s->target;

        const float bodyDb = body.cur / 100.0f * kBodyMaxDb;
        if (bodyDb > 0.002f) { updateShelf (bodyDb); lastBodyDb = bodyDb; }
        clipSettings = SoftClipper::makeSettings (clip.cur / 100.0f);
        needSnap = false;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        stepSmooth (attack, smoothCoef);
        stepSmooth (sustain, smoothCoef);
        stepSmooth (body, smoothCoef);
        stepSmooth (punch, smoothCoef);
        stepSmooth (clip, smoothCoef);
        stepSmooth (mix, smoothCoef);
        stepSmooth (inGain, smoothCoef);
        stepSmooth (outGain, smoothCoef);
        stepSmooth (bypassMix, smoothCoef);

        if (subCounter == 0)
        {
            const float bodyDb = body.cur / 100.0f * kBodyMaxDb;
            if (std::fabs (bodyDb - lastBodyDb) > 0.002f)
            {
                updateShelf (bodyDb);
                lastBodyDb = bodyDb;
            }
            clipSettings = SoftClipper::makeSettings (clip.cur / 100.0f);
        }
        subCounter = (subCounter + 1) % kSubBlock;

        // ---- entrada ----
        float raw[kMaxChannels] = { 0.0f, 0.0f };
        float x[kMaxChannels]   = { 0.0f, 0.0f };
        float detect = 0.0f;

        for (int c = 0; c < nch; ++c)
        {
            raw[c] = sanitize (channels[c][i]);
            x[c] = raw[c] * inGain.cur;
            detect = std::max (detect, std::fabs (x[c]));
        }

        // ---- deteccion de transitorios ----
        envFast = follow (envFast, detect, fastAtk, fastRel);
        envSlow = follow (envSlow, detect, slowAtk, slowRel);

        const float diffDb = 6.0206f * std::log2 ((envFast + 1.0e-6f) / (envSlow + 1.0e-6f));
        // Debajo de ~-60 dBFS se desvanece el efecto: evita bombear ruido de fondo.
        const float level = std::min (1.0f, std::max (envFast, envSlow) * 1000.0f);

        const float atkAmt = (attack.cur - 50.0f) / 50.0f;     // -1..+1
        const float susAmt = (sustain.cur - 50.0f) / 50.0f;    // -1..+1

        const float targetDb = (atkAmt * clampf (diffDb, 0.0f, kMaxShapeDb)
                              + susAmt * clampf (-diffDb, 0.0f, kMaxShapeDb)) * level;
        gainDbSmooth = flush (targetDb + (gainDbSmooth - targetDb) * gainCoef);
        const float shapeGain = dbToLin (gainDbSmooth);

        const float targetTransient = clampf (diffDb, 0.0f, 12.0f) / 12.0f * level;
        transientSmooth = flush (targetTransient + (transientSmooth - targetTransient) * gainCoef);

        const float punchAmt = punch.cur / 100.0f * kPunchGain * transientSmooth;
        const float mixV = mix.cur / 100.0f;
        const float bypassV = bypassMix.cur;

        // ---- salida por canal ----
        for (int c = 0; c < nch; ++c)
        {
            float y = x[c] * shapeGain;                         // ATTACK / SUSTAIN
            y = runBiquad (shelfCoefs, shelfState[c], y);       // BODY

            const float low = runBiquad (lowpassCoefs, lowpassState[c], x[c]);
            y += low * punchAmt;                                // PUNCH

            y = SoftClipper::process (y, clipSettings);         // CLIP

            float out = (x[c] + (y - x[c]) * mixV) * outGain.cur;   // MIX + OUTPUT
            out += (raw[c] - out) * bypassV;                         // BYPASS

            channels[c][i] = std::isfinite (out) ? out : 0.0f;
        }
    }
}
} // namespace impactor
