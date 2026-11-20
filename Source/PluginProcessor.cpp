#include "PluginProcessor.h"
#include "PluginEditor.h"

EQSceneProcessor::EQSceneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout EQSceneProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Low cut (high-pass)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { eqs::pid (eqs::lowCut, "freq"), 1 }, "Low Cut Freq",
        juce::NormalisableRange<float> (20.0f, 500.0f, 0.0f, 0.3f), 30.0f, "Hz"));

    // Four bells
    const float bellDefaults[] = { 300.0f, 400.0f, 1500.0f, 5000.0f };
    for (int b = 1; b <= 4; ++b)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eqs::pid (b, "freq"), 1 }, "Bell " + juce::String (b) + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), bellDefaults[b - 1], "Hz"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eqs::pid (b, "gain"), 1 }, "Bell " + juce::String (b) + " Gain",
            juce::NormalisableRange<float> (-18.0f, 18.0f), 0.0f, "dB"));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eqs::pid (b, "q"), 1 }, "Bell " + juce::String (b) + " Q",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f, 0.5f), 1.0f));
    }

    // High shelf
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { eqs::pid (eqs::highShelf, "freq"), 1 }, "High Shelf Freq",
        juce::NormalisableRange<float> (1000.0f, 20000.0f, 0.0f, 0.3f), 10000.0f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { eqs::pid (eqs::highShelf, "gain"), 1 }, "High Shelf Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f), 0.0f, "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { eqs::pid (eqs::highShelf, "q"), 1 }, "High Shelf Q",
        juce::NormalisableRange<float> (0.1f, 2.0f), 0.707f));

    // Output
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out", 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f, "dB"));

    return { params.begin(), params.end() };
}

void EQSceneProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    for (auto& f : filtersL) f.reset();
    for (auto& f : filtersR) f.reset();

    cached = {};
    updateFilters();
}

void EQSceneProcessor::updateFilters()
{
    for (int b = 0; b < eqs::numBands; ++b)
    {
        const float freq = getParam (eqs::pid (b, "freq"));
        const float gain = (b == eqs::lowCut) ? 0.0f : getParam (eqs::pid (b, "gain"));
        const float q    = (b == eqs::lowCut) ? 0.707f : getParam (eqs::pid (b, "q"));

        auto& c = cached[(size_t) b];
        if (c[0] == freq && c[1] == gain && c[2] == q)
            continue;

        c = { freq, gain, q };

        auto coeffs = eqs::makeBandCoeffs (b, currentSampleRate, freq, gain, q);
        filtersL[(size_t) b].coefficients = coeffs;
        filtersR[(size_t) b].coefficients = coeffs;
    }
}

void EQSceneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = 2; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    updateFilters();

    const int n = buffer.getNumSamples();

    if (buffer.getNumChannels() > 0)
    {
        auto* data = buffer.getWritePointer (0);
        for (auto& f : filtersL)
            for (int i = 0; i < n; ++i)
                data[i] = f.processSample (data[i]);
    }

    if (buffer.getNumChannels() > 1)
    {
        auto* data = buffer.getWritePointer (1);
        for (auto& f : filtersR)
            for (int i = 0; i < n; ++i)
                data[i] = f.processSample (data[i]);
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (getParam ("out")));
}

juce::AudioProcessorEditor* EQSceneProcessor::createEditor()
{
    return new EQSceneEditor (*this);
}

void EQSceneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void EQSceneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQSceneProcessor();
}
