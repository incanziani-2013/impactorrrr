#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "DSP/TransientProcessor.h"

/** IDs de parametros. NO cambiarlos nunca: los proyectos de FL Studio y los presets dependen de ellos. */
namespace ParamIDs
{
inline constexpr const char* attack     = "attack";
inline constexpr const char* sustain    = "sustain";
inline constexpr const char* body       = "body";
inline constexpr const char* punch      = "punch";
inline constexpr const char* clip       = "clip";
inline constexpr const char* mix        = "mix";
inline constexpr const char* inputGain  = "inputGain";
inline constexpr const char* outputGain = "outputGain";
inline constexpr const char* bypass     = "bypass";
} // namespace ParamIDs

class ImpactorAudioProcessor : public juce::AudioProcessor
{
public:
    ImpactorAudioProcessor();
    ~ImpactorAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;   // evita ocultar la version double

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "IMPACTOR"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    /** Pico de salida del ultimo bloque (0..N, lineal). Lo lee la interfaz. */
    float getOutputLevel() const noexcept { return outputPeak.load (std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pBody = nullptr;
    std::atomic<float>* pPunch = nullptr;
    std::atomic<float>* pClip = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pOutputGain = nullptr;
    std::atomic<float>* pBypass = nullptr;

    impactor::TransientProcessor engine;
    std::atomic<float> outputPeak { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpactorAudioProcessor)
};
