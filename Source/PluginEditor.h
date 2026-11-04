#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

#include "PluginProcessor.h"

/**
 * Editor de la ETAPA 1: sencillo pero 100 % funcional (todos los controles
 * estan conectados al APVTS). La interfaz profesional llega en la ETAPA 2.
 */
class ImpactorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit ImpactorAudioProcessorEditor (ImpactorAudioProcessor&);
    ~ImpactorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    ImpactorAudioProcessor& processor;

    std::array<Knob, 8> knobs;
    juce::ToggleButton bypassButton { "BYPASS" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    float displayLevel = 0.0f;
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImpactorAudioProcessorEditor)
};
