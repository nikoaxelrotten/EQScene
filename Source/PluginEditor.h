#pragma once

#include "PluginProcessor.h"

class EQSceneEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit EQSceneEditor (EQSceneProcessor&);
    ~EQSceneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

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

    EQSceneProcessor& proc;

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

    int draggedBand = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQSceneEditor)
};
