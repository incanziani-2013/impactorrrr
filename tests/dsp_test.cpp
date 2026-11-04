// Test del motor DSP de IMPACTOR (sin JUCE).
// Compilar:  g++ -std=c++17 -O2 -I Source tests/dsp_test.cpp Source/DSP/*.cpp -o dsp_test
#include "DSP/TransientProcessor.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

using impactor::TransientProcessor;

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (cond) std::printf ("  [OK]   %s\n", msg);                          \
        else { std::printf ("  [FAIL] %s\n", msg); ++failures; }               \
    } while (0)

static const double kSr = 44100.0;
static const double kPi = 3.14159265358979323846;

// "Bombo" sintetico: 50 ms de silencio, golpe de 100 Hz con caida exponencial.
static std::vector<float> makeKick (int n, float amp = 0.5f)
{
    std::vector<float> v ((size_t) n, 0.0f);
    const int onset = (int) (0.05 * kSr);
    for (int i = onset; i < n; ++i)
    {
        const double t = (double) (i - onset) / kSr;
        v[(size_t) i] = (float) (amp * std::exp (-t / 0.12) * std::sin (2.0 * kPi * 100.0 * t));
    }
    return v;
}

static std::vector<float> run (const std::vector<float>& in, const TransientProcessor::Params& p, int block = 256)
{
    TransientProcessor dsp;
    dsp.prepare (kSr);
    dsp.setParams (p);

    std::vector<float> out = in;
    for (size_t pos = 0; pos < out.size(); pos += (size_t) block)
    {
        int n = (int) std::min ((size_t) block, out.size() - pos);
        float* ch = out.data() + pos;
        float* const chans[1] = { ch };
        dsp.process (chans, 1, n);
    }
    return out;
}

static float peak (const std::vector<float>& v, double t0, double t1)
{
    float m = 0.0f;
    for (int i = (int) (t0 * kSr); i < (int) (t1 * kSr) && i < (int) v.size(); ++i)
        m = std::max (m, std::fabs (v[(size_t) i]));
    return m;
}

static double energy (const std::vector<float>& v, double t0, double t1)
{
    double e = 0.0;
    for (int i = (int) (t0 * kSr); i < (int) (t1 * kSr) && i < (int) v.size(); ++i)
        e += (double) v[(size_t) i] * v[(size_t) i];
    return e;
}

static double db (double ratio) { return 20.0 * std::log10 (ratio); }

int main()
{
    const int N = (int) (0.6 * kSr);
    const auto kick = makeKick (N);
    TransientProcessor::Params neutral;

    std::printf ("1) Parametros neutros = senal intacta\n");
    {
        auto out = run (kick, neutral);
        double maxErr = 0.0;
        for (size_t i = 0; i < kick.size(); ++i)
            maxErr = std::max (maxErr, (double) std::fabs (out[i] - kick[i]));
        std::printf ("       error maximo: %.2e\n", maxErr);
        CHECK (maxErr < 1.0e-5, "salida identica a la entrada con ajustes neutros");
    }

    std::printf ("2) ATTACK realza el golpe\n");
    {
        auto p = neutral; p.attack = 100.0f;
        auto out = run (kick, p);
        const double gain = db (peak (out, 0.05, 0.08) / peak (kick, 0.05, 0.08));
        std::printf ("       cambio de pico en el ataque: %+.1f dB\n", gain);
        CHECK (gain > 2.0, "ATTACK=100 sube el pico del ataque mas de 2 dB");

        p.attack = 0.0f;
        out = run (kick, p);
        const double gain2 = db (peak (out, 0.05, 0.08) / peak (kick, 0.05, 0.08));
        std::printf ("       cambio de pico en el ataque: %+.1f dB\n", gain2);
        CHECK (gain2 < -2.0, "ATTACK=0 baja el pico del ataque mas de 2 dB");
    }

    std::printf ("3) SUSTAIN controla la cola\n");
    {
        auto p = neutral; p.sustain = 0.0f;
        auto out = run (kick, p);
        const double g0 = db (energy (out, 0.2, 0.5) / energy (kick, 0.2, 0.5));
        std::printf ("       cola con SUSTAIN=0: %+.1f dB\n", g0);
        CHECK (g0 < -2.0, "SUSTAIN=0 acorta la cola");

        p.sustain = 100.0f;
        out = run (kick, p);
        const double g1 = db (energy (out, 0.2, 0.5) / energy (kick, 0.2, 0.5));
        std::printf ("       cola con SUSTAIN=100: %+.1f dB\n", g1);
        CHECK (g1 > 2.0, "SUSTAIN=100 alarga la cola");
    }

    std::printf ("4) BODY y PUNCH suman energia grave\n");
    {
        auto p = neutral; p.body = 100.0f;
        auto out = run (kick, p);
        const double g = db (energy (out, 0.05, 0.4) / energy (kick, 0.05, 0.4));
        std::printf ("       BODY=100: %+.1f dB de energia\n", g);
        CHECK (g > 3.0, "BODY=100 aumenta la energia del grave");

        p = neutral; p.punch = 100.0f;
        out = run (kick, p);
        const double g2 = db (peak (out, 0.05, 0.1) / peak (kick, 0.05, 0.1));
        std::printf ("       PUNCH=100: %+.1f dB de pico\n", g2);
        CHECK (g2 > 1.0, "PUNCH=100 aumenta el pico del golpe");
    }

    std::printf ("5) CLIP limita y satura\n");
    {
        auto loud = makeKick (N, 3.0f);
        auto p = neutral; p.clip = 100.0f;
        auto out = run (loud, p);
        const float pk = peak (out, 0.0, 0.6);
        std::printf ("       pico con entrada de 3.0 y CLIP=100: %.3f\n", pk);
        CHECK (pk <= 1.001f, "CLIP=100 deja el pico en <= 1.0");
    }

    std::printf ("6) MIX\n");
    {
        auto p = neutral; p.attack = 100.0f; p.clip = 100.0f; p.mix = 0.0f;
        auto out = run (kick, p);
        double maxErr = 0.0;
        for (size_t i = 0; i < kick.size(); ++i)
            maxErr = std::max (maxErr, (double) std::fabs (out[i] - kick[i]));
        CHECK (maxErr < 1.0e-5, "MIX=0 devuelve la senal seca");
    }

    std::printf ("7) BYPASS\n");
    {
        auto p = neutral; p.attack = 100.0f; p.clip = 100.0f; p.outputGainDb = 12.0f; p.bypass = true;
        auto out = run (kick, p);
        double maxErr = 0.0;
        // primeros 20 ms: el crossfade todavia esta llegando (arranque suave)
        for (size_t i = (size_t) (0.2 * kSr); i < kick.size(); ++i)
            maxErr = std::max (maxErr, (double) std::fabs (out[i] - kick[i]));
        std::printf ("       error: %.2e\n", maxErr);
        CHECK (maxErr < 1.0e-4, "BYPASS devuelve la entrada original");
    }

    std::printf ("8) Ganancias de entrada y salida\n");
    {
        auto p = neutral; p.inputGainDb = 6.0f; p.outputGainDb = -6.0f;
        auto out = run (kick, p);
        double maxErr = 0.0;
        for (size_t i = 0; i < kick.size(); ++i)
            maxErr = std::max (maxErr, (double) std::fabs (out[i] - kick[i]));
        CHECK (maxErr < 5.0e-3, "+6 dB entrada y -6 dB salida se compensan");
    }

    std::printf ("9) Robustez: silencio, NaN, Inf, valores enormes\n");
    {
        std::vector<float> silence ((size_t) N, 0.0f);
        auto p = neutral; p.attack = 100.0f; p.sustain = 100.0f; p.body = 100.0f; p.punch = 100.0f; p.clip = 100.0f;
        auto out = run (silence, p);
        bool allZero = true;
        for (float v : out) if (v != 0.0f) { allZero = false; break; }
        CHECK (allZero, "silencio de entrada = silencio de salida (sin ruido ni denormales)");

        std::vector<float> bad = kick;
        bad[2000] = std::numeric_limits<float>::quiet_NaN();
        bad[2001] = std::numeric_limits<float>::infinity();
        bad[2002] = -std::numeric_limits<float>::infinity();
        bad[2003] = 1.0e30f;
        out = run (bad, p);
        bool finite = true;
        for (float v : out) if (! std::isfinite (v)) { finite = false; break; }
        CHECK (finite, "NaN/Inf/1e30 en la entrada no producen NaN/Inf en la salida");

        // Recuperacion: con NaN/Inf aislados (que se tratan como 0), la cola debe coincidir
        // con la de una corrida limpia con los mismos parametros.
        std::vector<float> bad2 = kick;
        bad2[2000] = std::numeric_limits<float>::quiet_NaN();
        bad2[2001] = std::numeric_limits<float>::infinity();
        const auto cleanOut = run (kick, p);
        const auto badOut = run (bad2, p);
        float maxDiff = 0.0f;
        for (size_t i = (size_t) (0.3 * kSr); i < badOut.size(); ++i)
            maxDiff = std::max (maxDiff, std::fabs (badOut[i] - cleanOut[i]));
        std::printf ("       diferencia maxima en la cola: %.2e\n", maxDiff);
        CHECK (maxDiff < 1.0e-3f, "el motor se recupera despues de NaN/Inf aislados");
    }

    std::printf ("10) Cambios bruscos de parametros no hacen clicks\n");
    {
        TransientProcessor dsp;
        dsp.prepare (kSr);
        std::vector<float> sine ((size_t) (0.5 * kSr));
        for (size_t i = 0; i < sine.size(); ++i)
            sine[i] = 0.3f * (float) std::sin (2.0 * kPi * 220.0 * (double) i / kSr);

        float maxJump = 0.0f, prev = 0.0f;
        for (size_t pos = 0; pos < sine.size(); pos += 128)
        {
            auto p = neutral;
            p.body = ((pos / 128) % 2) ? 100.0f : 0.0f;       // salto total cada bloque
            p.outputGainDb = ((pos / 128) % 2) ? -12.0f : 0.0f;
            dsp.setParams (p);
            float* ch = sine.data() + pos;
            float* const chans[1] = { ch };
            dsp.process (chans, 1, (int) std::min ((size_t) 128, sine.size() - pos));
            for (size_t i = pos; i < std::min (pos + 128, sine.size()); ++i)
            {
                if (i > 0) maxJump = std::max (maxJump, std::fabs (sine[i] - prev));
                prev = sine[i];
            }
        }
        std::printf ("       salto maximo entre muestras: %.4f (seno de 220 Hz y 0.3: normal ~0.01)\n", maxJump);
        CHECK (maxJump < 0.08f, "sin saltos bruscos al cambiar parametros de golpe");
    }

    std::printf ("11) Estereo\n");
    {
        TransientProcessor dsp;
        dsp.prepare (kSr);
        auto p = neutral; p.attack = 80.0f; p.punch = 50.0f;
        dsp.setParams (p);

        auto l = kick, r = kick;
        for (auto& v : r) v *= 0.5f;
        float* const chans[2] = { l.data(), r.data() };
        for (size_t pos = 0; pos < l.size(); pos += 512)
        {
            float* const blk[2] = { l.data() + pos, r.data() + pos };
            dsp.process (blk, 2, (int) std::min ((size_t) 512, l.size() - pos));
        }
        (void) chans;
        const double ratio = peak (r, 0.05, 0.3) / peak (l, 0.05, 0.3);
        std::printf ("       relacion R/L: %.3f (esperado ~0.5)\n", ratio);
        CHECK (std::fabs (ratio - 0.5) < 0.1, "la imagen estereo se mantiene (deteccion enlazada)");
    }

    std::printf ("\n%s (%d fallos)\n", failures == 0 ? "TODOS LOS TESTS PASARON" : "HAY TESTS QUE FALLAN", failures);
    return failures == 0 ? 0 : 1;
}
