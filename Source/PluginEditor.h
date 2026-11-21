#pragma once

#include "PluginProcessor.h"

// Draws the rotary knobs in the style of the reference image.
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
};

class EQSceneEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit EQSceneEditor (EQSceneProcessor&);
    ~EQSceneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override { repaint(); }

    juce::Rectangle<float> graphBounds() const;
    juce::Point<float> nodePosition (int band) const;
    int findNodeAt (juce::Point<float> p) const;
    void setParamValue (const juce::String& id, float value);
    void beginGesture (int band, bool begin);
    float getParam (const juce::String& id) const;
    juce::String describeBand (int band) const;
    std::array<float, eqs::numZones> zoneDeviations (double sr) const;

    EQSceneProcessor& proc;

    // Declared before the knobs so it is destroyed after them
    KnobLookAndFeel knobLook;

    struct BandUI
    {
        juce::Label title;
        juce::Slider sFreq, sGain, sQ;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aFreq, aGain, aQ;
    };

    std::array<BandUI, eqs::numBands> bandUI;
    juce::Label outTitle;
    juce::Slider sOut;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aOut;

    juce::TextButton inButton, outButton;
    bool showIn = true;
    bool showOut = true;

    juce::ComboBox instrumentBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aInstrument;

    int draggedBand = -1;
    int hoverBand = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQSceneEditor)
};
