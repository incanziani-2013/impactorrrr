#include "PluginEditor.h"

namespace
{
struct KnobSpec
{
    const char* paramId;
    const char* title;
};

const KnobSpec kSpecs[8] = {
    { ParamIDs::inputGain,  "INPUT"   },
    { ParamIDs::attack,     "ATTACK"  },
    { ParamIDs::sustain,    "SUSTAIN" },
    { ParamIDs::body,       "BODY"    },
    { ParamIDs::punch,      "PUNCH"   },
    { ParamIDs::clip,       "CLIP"    },
    { ParamIDs::mix,        "MIX"     },
    { ParamIDs::outputGain, "OUTPUT"  },
};

const juce::Colour kBackground (0xff15181d);
const juce::Colour kCyan (0xff2fd6e8);
const juce::Colour kBlue (0xff2f7bff);
} // namespace

ImpactorAudioProcessorEditor::ImpactorAudioProcessorEditor (ImpactorAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];

        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        k.slider.setColour (juce::Slider::rotarySliderFillColourId, kCyan);
        k.slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setDoubleClickReturnValue (true, p.apvts.getParameter (kSpecs[i].paramId)->convertFrom0to1 (
                                                      p.apvts.getParameter (kSpecs[i].paramId)->getDefaultValue()));
        addAndMakeVisible (k.slider);

        k.label.setText (kSpecs[i].title, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setColour (juce::Label::textColourId, kCyan);
        k.label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            p.apvts, kSpecs[i].paramId, k.slider);
    }

    bypassButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    bypassButton.setColour (juce::ToggleButton::tickColourId, kCyan);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        p.apvts, ParamIDs::bypass, bypassButton);

    setSize (780, 360);
    startTimerHz (30);
}

ImpactorAudioProcessorEditor::~ImpactorAudioProcessorEditor()
{
    stopTimer();
}

void ImpactorAudioProcessorEditor::timerCallback()
{
    const float level = processor.getOutputLevel();
    displayLevel = juce::jmax (level, displayLevel * 0.85f);   // caida suave del medidor
    repaint (meterBounds);
}

void ImpactorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff262b33), 0.0f, 0.0f,
                                             juce::Colour (0xff15181d), 0.0f, (float) getHeight(), false));
    g.fillRect (getLocalBounds().removeFromTop (70));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("IMPACTOR", 20, 12, 300, 36, juce::Justification::centredLeft);

    g.setColour (kCyan);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("1.0 PRO  -  Transient Shaper", 22, 44, 300, 18, juce::Justification::centredLeft);

    // Medidor de salida
    g.setColour (juce::Colour (0xff0b0d10));
    g.fillRoundedRectangle (meterBounds.toFloat(), 4.0f);

    const float db = juce::Decibels::gainToDecibels (displayLevel, -60.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
    auto fill = meterBounds.toFloat().reduced (2.0f);
    fill.setWidth (fill.getWidth() * norm);

    g.setGradientFill (juce::ColourGradient (kBlue, fill.getX(), 0.0f, kCyan, meterBounds.getRight(), 0.0f, false));
    g.fillRoundedRectangle (fill, 3.0f);

    g.setColour (displayLevel > 1.0f ? juce::Colours::red : juce::Colours::grey);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("OUT " + juce::String (db, 1) + " dB", meterBounds.withY (meterBounds.getBottom() + 2).withHeight (14),
                juce::Justification::centredLeft);
}

void ImpactorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (60);

    auto top = area.removeFromTop (area.getHeight() - 60);
    const int w = top.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto cell = top.removeFromLeft (w);
        k.label.setBounds (cell.removeFromTop (22));
        k.slider.setBounds (cell);
    }

    auto bottom = area;
    bypassButton.setBounds (bottom.removeFromLeft (110).withHeight (30));
    meterBounds = bottom.removeFromTop (18).withTrimmedLeft (20).withTrimmedRight (10);
}
