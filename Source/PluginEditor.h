#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;
    constexpr float maxDb = 18.0f;

    const juce::Colour bandColours[eqs::numBands] = {
        juce::Colour (0xff9aa4b2), // low cut
        juce::Colour (0xff4fd1c5),
        juce::Colour (0xff8bd450),
        juce::Colour (0xffffc857),
        juce::Colour (0xffff6b6b),
        juce::Colour (0xff7aa2ff), // high shelf
    };

    const char* bandNames[eqs::numBands] = { "Low Cut", "Bell 1", "Bell 2", "Bell 3", "Bell 4", "High Shelf" };

    struct Zone { const char* name; float f1, f2; juce::uint32 colour; };
    const Zone zones[] = {
        { "Retumbe / graves", 20.0f, 110.0f, 0xff1f5f7a },
        { "Cuerpo", 150.0f, 500.0f, 0xff1e7a6e },
        { "Barro / opacidad", 300.0f, 800.0f, 0xff4f7a22 },
        { "Nasal / cart\xc3\xb3n", 800.0f, 3000.0f, 0xff8a7a1a },
        { "Presencia", 2000.0f, 5000.0f, 0xff6a2e8a },
        { "Dureza", 4000.0f, 8000.0f, 0xff7a1d4d },
        { "Aire", 8000.0f, 16000.0f, 0xff3a3a8a },
    };

    float freqToX (float f, juce::Rectangle<float> r)
    {
        return r.getX() + std::log (f / minFreq) / std::log (maxFreq / minFreq) * r.getWidth();
    }

    float xToFreq (float x, juce::Rectangle<float> r)
    {
        return minFreq * std::pow (maxFreq / minFreq, (x - r.getX()) / r.getWidth());
    }

    float gainToY (float db, juce::Rectangle<float> r)
    {
        return r.getCentreY() - (db / maxDb) * (r.getHeight() * 0.45f);
    }

    float yToGain (float y, juce::Rectangle<float> r)
    {
        return (r.getCentreY() - y) / (r.getHeight() * 0.45f) * maxDb;
    }
}

EQSceneEditor::EQSceneEditor (EQSceneProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (980, 600);

    auto setupRotary = [] (juce::Slider& s, juce::Colour c)
    {
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        s.setColour (juce::Slider::rotarySliderFillColourId, c);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff222831));
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    };
