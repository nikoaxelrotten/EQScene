#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace eqs
{
    // Band layout: 0 = low cut, 1-4 = bells, 5 = high shelf
    constexpr int numBands = 6;
    constexpr int lowCut = 0;
    constexpr int highShelf = 5;

    inline juce::String pid (int band, const char* what)
    {
        return "b" + juce::String (band) + "_" + what;
    }

    inline juce::dsp::IIR::Coefficients<float>::Ptr makeBandCoeffs (int band, double sr,
                                                                  float freq, float gainDb, float q)
    {
        using C = juce::dsp::IIR::Coefficients<float>;

        if (band == lowCut)
            return C::makeHighPass (sr, freq, 0.707f);

        const auto gainFactor = juce::Decibels::decibelsToGain (gainDb);

        if (band == highShelf)
            return C::makeHighShelf (sr, freq, q, gainFactor);

        return C::makePeakFilter (sr, freq, q, gainFactor);
    }
}

class EQSceneProcessor : public juce::AudioProcessor
{
public:
    EQSceneProcessor();
    ~EQSceneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EQScene"; }
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

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    float getParam (const juce::String& id) const
    {
        return apvts.getRawParameterValue (id)->load();
    }

    std::array<juce::dsp::IIR::Filter<float>, eqs::numBands> filtersL, filtersR;
    std::array<std::array<float, 3>, eqs::numBands> cached {};
    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQSceneProcessor)
};
