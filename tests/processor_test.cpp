// Test del procesador JUCE: parametros, estado guardado/restaurado y audio real.
// Se compila con:  cmake --build build --target impactor_processor_test
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <cstdio>

#include "PluginProcessor.h"

static int failures = 0;
#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (cond) std::printf ("  [OK]   %s\n", msg);                          \
        else { std::printf ("  [FAIL] %s\n", msg); ++failures; }               \
    } while (0)

static void setParam (ImpactorAudioProcessor& p, const char* id, float realValue)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

static float peakOf (const juce::AudioBuffer<float>& b)
{
    return b.getMagnitude (0, b.getNumSamples());
}

static juce::AudioBuffer<float> makeKick (int n, double sr)
{
    juce::AudioBuffer<float> b (2, n);
    b.clear();
    const int onset = (int) (0.02 * sr);
    for (int i = onset; i < n; ++i)
    {
        const double t = (double) (i - onset) / sr;
        const float v = (float) (0.5 * std::exp (-t / 0.1) * std::sin (2.0 * 3.14159265358979 * 90.0 * t));
        b.setSample (0, i, v);
        b.setSample (1, i, v);
    }
    return b;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const double sr = 44100.0;
    const int n = 22050;
    juce::MidiBuffer midi;

    std::printf ("1) Parametros\n");
    ImpactorAudioProcessor proc;
    const char* ids[] = { "attack", "sustain", "body", "punch", "clip", "mix", "inputGain", "outputGain", "bypass" };
    bool allExist = true;
    for (auto* id : ids)
        if (proc.apvts.getParameter (id) == nullptr) allExist = false;
    CHECK (allExist, "existen los 9 IDs de parametros");
    CHECK (proc.getParameters().size() == 9, "hay exactamente 9 parametros automatizables");
    CHECK (proc.getBypassParameter() != nullptr, "bypass conectado al bypass del host");

    std::printf ("2) Audio\n");
    proc.setPlayConfigDetails (2, 2, sr, 512);
    proc.prepareToPlay (sr, 512);

    auto process = [&] (ImpactorAudioProcessor& pr, juce::AudioBuffer<float> buf)
    {
        for (int pos = 0; pos < n; pos += 512)
        {
            juce::AudioBuffer<float> blk (buf.getArrayOfWritePointers(), 2, pos, juce::jmin (512, n - pos));
            pr.processBlock (blk, midi);
        }
        return buf;
    };

    const auto kick = makeKick (n, sr);
    auto neutral = process (proc, kick);
    double err = 0.0;
    for (int i = 0; i < n; ++i) err = std::max (err, (double) std::fabs (neutral.getSample (0, i) - kick.getSample (0, i)));
    CHECK (err < 1.0e-5, "ajustes por defecto = audio intacto");

    setParam (proc, "attack", 100.0f);
    proc.prepareToPlay (sr, 512);
    auto boosted = process (proc, kick);
    std::printf ("       pico original %.3f -> con ATTACK=100 %.3f\n", peakOf (kick), peakOf (boosted));
    CHECK (peakOf (boosted) > peakOf (kick) * 1.3f, "mover ATTACK en el APVTS cambia el audio");
    CHECK (proc.getOutputLevel() >= 0.0f, "el medidor de salida se actualiza");

    std::printf ("3) Estado (guardar / restaurar proyecto)\n");
    setParam (proc, "body", 63.0f);
    setParam (proc, "mix", 40.0f);
    setParam (proc, "bypass", 1.0f);
    juce::MemoryBlock state;
    proc.getStateInformation (state);

    ImpactorAudioProcessor proc2;
    proc2.setStateInformation (state.getData(), (int) state.getSize());
    auto val = [&] (const char* id) { return proc2.apvts.getRawParameterValue (id)->load(); };
    std::printf ("       restaurado: attack=%.1f body=%.1f mix=%.1f bypass=%.0f\n", val ("attack"), val ("body"), val ("mix"), val ("bypass"));
    CHECK (std::fabs (val ("attack") - 100.0f) < 0.11f, "attack se restaura");
    CHECK (std::fabs (val ("body") - 63.0f) < 0.11f, "body se restaura");
    CHECK (std::fabs (val ("mix") - 40.0f) < 0.11f, "mix se restaura");
    CHECK (val ("bypass") > 0.5f, "bypass se restaura");

    char garbage[] = "esto no es un estado valido";
    proc2.setStateInformation (garbage, (int) sizeof (garbage));
    CHECK (std::fabs (val ("body") - 63.0f) < 0.11f, "un estado corrupto no rompe nada");

    std::printf ("4) Layouts de canales\n");
    {
        juce::AudioProcessor::BusesLayout mono, stereo, bad;
        mono.inputBuses.add (juce::AudioChannelSet::mono());   mono.outputBuses.add (juce::AudioChannelSet::mono());
        stereo.inputBuses.add (juce::AudioChannelSet::stereo()); stereo.outputBuses.add (juce::AudioChannelSet::stereo());
        bad.inputBuses.add (juce::AudioChannelSet::mono());    bad.outputBuses.add (juce::AudioChannelSet::stereo());
        CHECK (proc.checkBusesLayoutSupported (mono), "mono -> mono soportado");
        CHECK (proc.checkBusesLayoutSupported (stereo), "estereo -> estereo soportado");
        CHECK (! proc.checkBusesLayoutSupported (bad), "mono -> estereo rechazado");
    }

    std::printf ("\n%s (%d fallos)\n", failures == 0 ? "TODOS LOS TESTS PASARON" : "HAY TESTS QUE FALLAN", failures);
    return failures == 0 ? 0 : 1;
}
