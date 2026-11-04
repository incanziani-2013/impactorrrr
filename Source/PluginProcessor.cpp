#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kParamVersion = 1;

juce::ParameterID pid (const char* id) { return juce::ParameterID { id, kParamVersion }; }
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout ImpactorAudioProcessor::createParameterLayout()
{
    using juce::AudioParameterBool;
    using juce::AudioParameterFloat;
    using juce::AudioParameterFloatAttributes;
    using juce::NormalisableRange;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const NormalisableRange<float> percent (0.0f, 100.0f, 0.1f);
    const NormalisableRange<float> gainDb (-24.0f, 24.0f, 0.1f);

    // attack / sustain: 50 = neutro, <50 reduce, >50 realza
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::attack),  "Attack",  percent, 50.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::sustain), "Sustain", percent, 50.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    // body / punch / clip: 0 = apagado
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::body),  "Body",  percent, 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::punch), "Punch", percent, 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::clip),  "Clip",  percent, 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::mix),   "Mix",   percent, 100.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::inputGain),  "Input Gain",  gainDb, 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<AudioParameterFloat> (pid (ParamIDs::outputGain), "Output Gain", gainDb, 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterBool> (pid (ParamIDs::bypass), "Bypass", false));

    return layout;
}

ImpactorAudioProcessor::ImpactorAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "IMPACTOR_STATE", createParameterLayout())
{
    pAttack     = apvts.getRawParameterValue (ParamIDs::attack);
    pSustain    = apvts.getRawParameterValue (ParamIDs::sustain);
    pBody       = apvts.getRawParameterValue (ParamIDs::body);
    pPunch      = apvts.getRawParameterValue (ParamIDs::punch);
    pClip       = apvts.getRawParameterValue (ParamIDs::clip);
    pMix        = apvts.getRawParameterValue (ParamIDs::mix);
    pInputGain  = apvts.getRawParameterValue (ParamIDs::inputGain);
    pOutputGain = apvts.getRawParameterValue (ParamIDs::outputGain);
    pBypass     = apvts.getRawParameterValue (ParamIDs::bypass);
}

void ImpactorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    outputPeak.store (0.0f);
}

bool ImpactorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

void ImpactorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    impactor::TransientProcessor::Params p;
    p.attack       = pAttack->load();
    p.sustain      = pSustain->load();
    p.body         = pBody->load();
    p.punch        = pPunch->load();
    p.clip         = pClip->load();
    p.mix          = pMix->load();
    p.inputGainDb  = pInputGain->load();
    p.outputGainDb = pOutputGain->load();
    p.bypass       = pBypass->load() > 0.5f;
    engine.setParams (p);

    const int channelsToProcess = juce::jmin (numIn, buffer.getNumChannels());
    engine.process (buffer.getArrayOfWritePointers(), channelsToProcess, numSamples);

    float peak = 0.0f;
    for (int ch = 0; ch < channelsToProcess; ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
    outputPeak.store (peak, std::memory_order_relaxed);
}

juce::AudioProcessorParameter* ImpactorAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParamIDs::bypass);
}

juce::AudioProcessorEditor* ImpactorAudioProcessor::createEditor()
{
    return new ImpactorAudioProcessorEditor (*this);
}

void ImpactorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ImpactorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Punto de entrada que JUCE busca para crear el plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImpactorAudioProcessor();
}
