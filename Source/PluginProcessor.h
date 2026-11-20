#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

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

// Collects samples, runs an FFT on each block and keeps the magnitudes in dB.
class SpectrumAnalyzer
{
public:
    static constexpr int order = 11;
    static constexpr int fftSize = 1 << order;
    static constexpr int numBins = fftSize / 2;

    SpectrumAnalyzer() : fft (order)
    {
        for (int i = 0; i < fftSize; ++i)
            window[(size_t) i] = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                                          * (float) i / (float) (fftSize - 1)));
        magnitudesDb.fill (-120.0f);
    }

    void pushSample (float s)
    {
        fifo[(size_t) fifoIndex++] = s;

        if (fifoIndex == fftSize)
        {
            computeSpectrum();
            fifoIndex = 0;
        }
    }

    void copyMagnitudes (std::array<float, numBins>& dest)
    {
        const juce::SpinLock::ScopedLockType lock (spinLock);
        dest = magnitudesDb;
    }

private:
    void computeSpectrum()
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = fifo[(size_t) i] * window[(size_t) i];

        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const juce::SpinLock::ScopedLockType lock (spinLock);
        for (int i = 0; i < numBins; ++i)
        {
            const float mag = fftData[(size_t) i] / (float) numBins;
            magnitudesDb[(size_t) i] = 20.0f * std::log10 (std::max (mag, 1.0e-6f));
        }
    }

    juce::dsp::FFT fft;
    std::array<float, fftSize> window {};
    std::array<float, fftSize> fifo {};
    std::array<float, fftSize * 2> fftData {};
    std::array<float, numBins> magnitudesDb {};
    int fifoIndex = 0;
    juce::SpinLock spinLock;
};

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
    SpectrumAnalyzer inAnalyzer, outAnalyzer;

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
