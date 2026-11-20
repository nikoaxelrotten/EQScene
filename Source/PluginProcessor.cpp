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

    for (int b = 0; b < eqs::numBands; ++b)
    {
        auto& ui = bandUI[(size_t) b];
        const auto colour = bandColours[b];

        ui.title.setText (bandNames[b], juce::dontSendNotification);
        ui.title.setJustificationType (juce::Justification::centred);
        ui.title.setColour (juce::Label::textColourId, colour);
        addAndMakeVisible (ui.title);

        setupRotary (ui.sFreq, colour);
        addAndMakeVisible (ui.sFreq);
        ui.aFreq = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, eqs::pid (b, "freq"), ui.sFreq);

        if (b != eqs::lowCut)
        {
            setupRotary (ui.sGain, colour);
            addAndMakeVisible (ui.sGain);
            ui.aGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                proc.apvts, eqs::pid (b, "gain"), ui.sGain);

            setupRotary (ui.sQ, colour);
            addAndMakeVisible (ui.sQ);
            ui.aQ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                proc.apvts, eqs::pid (b, "q"), ui.sQ);
        }
    }

    outTitle.setText ("Output", juce::dontSendNotification);
    outTitle.setJustificationType (juce::Justification::centred);
    outTitle.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (outTitle);

    setupRotary (sOut, juce::Colours::white);
    addAndMakeVisible (sOut);
    aOut = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, "out", sOut);

    startTimerHz (30);
}

EQSceneEditor::~EQSceneEditor()
{
    stopTimer();
}

float EQSceneEditor::getParam (const juce::String& id) const
{
    return proc.apvts.getRawParameterValue (id)->load();
}

juce::Rectangle<float> EQSceneEditor::graphBounds() const
{
    return { 12.0f, 48.0f, (float) getWidth() - 24.0f, 320.0f };
}

juce::Point<float> EQSceneEditor::nodePosition (int band) const
{
    const auto r = graphBounds();
    const float freq = getParam (eqs::pid (band, "freq"));
    const float gain = (band == eqs::lowCut) ? 0.0f : getParam (eqs::pid (band, "gain"));
    return { freqToX (freq, r), gainToY (gain, r) };
}

int EQSceneEditor::findNodeAt (juce::Point<float> p) const
{
    int best = -1;
    float bestDist = 16.0f;

    for (int b = 0; b < eqs::numBands; ++b)
    {
        const float d = nodePosition (b).getDistanceFrom (p);
        if (d < bestDist)
        {
            bestDist = d;
            best = b;
        }
    }
    return best;
}

void EQSceneEditor::setParamValue (const juce::String& id, float value)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void EQSceneEditor::beginGesture (int band, bool begin)
{
    const char* parts[] = { "freq", "gain", "q" };
    for (auto* part : parts)
    {
        if (auto* p = proc.apvts.getParameter (eqs::pid (band, part)))
        {
            if (begin) p->beginChangeGesture();
            else       p->endChangeGesture();
        }
    }
}

void EQSceneEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0d0f14));

    const auto area = graphBounds();

    // Title
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("EQScene", 14, 10, 200, 26, juce::Justification::centredLeft);

    // Frequency zones with soft gradients
    for (const auto& z : zones)
    {
        const float x1 = freqToX (z.f1, area);
        const float x2 = freqToX (z.f2, area);
        const auto c = juce::Colour (z.colour);

        juce::ColourGradient grad (c.withAlpha (0.55f), 0.0f, area.getY(),
                                   c.withAlpha (0.0f), 0.0f, area.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (juce::Rectangle<float> (x1, area.getY(), x2 - x1, area.getHeight()));

        g.setColour (c.brighter (0.6f));
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (z.name, juce::Rectangle<float> (x1, area.getY() + 4, x2 - x1, 16),
                    juce::Justification::centred, false);
    }

    // Grid
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (float db = -18.0f; db <= 18.0f; db += 6.0f)
        g.drawHorizontalLine ((int) gainToY (db, area), area.getX(), area.getRight());

    const float gridFreqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (auto f : gridFreqs)
        g.drawVerticalLine ((int) freqToX (f, area), area.getY(), area.getBottom());

    // Zero line
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine ((int) gainToY (0.0f, area), area.getX(), area.getRight());

    // Coefficients for the current parameters
    const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
    std::array<juce::dsp::IIR::Coefficients<float>::Ptr, eqs::numBands> coefs;
    for (int b = 0; b < eqs::numBands; ++b)
    {
        const float freq = getParam (eqs::pid (b, "freq"));
        const float gain = (b == eqs::lowCut) ? 0.0f : getParam (eqs::pid (b, "gain"));
        const float q    = (b == eqs::lowCut) ? 0.707f : getParam (eqs::pid (b, "q"));
        coefs[(size_t) b] = eqs::makeBandCoeffs (b, sr, freq, gain, q);
    }

    // Combined response curve
    juce::Path curve;
    constexpr int points = 320;
    for (int i = 0; i < points; ++i)
    {
        const float f = minFreq * std::pow (maxFreq / minFreq, i / (float) (points - 1));

        float db = 0.0f;
        for (auto& c : coefs)
        {
            const float mag = c->getMagnitudeForFrequency (f, sr);
            db += 20.0f * std::log10 (std::max (mag, 1.0e-6f));
        }

        const float x = freqToX (f, area);
        const float y = std::clamp (gainToY (db, area), area.getY(), area.getBottom());

        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    // Filled area under the curve
    juce::Path fill (curve);
    const float zeroY = gainToY (0.0f, area);
    fill.lineTo (area.getRight(), zeroY);
    fill.lineTo (area.getX(), zeroY);
    fill.closeSubPath();

    juce::ColourGradient fillGrad (juce::Colour (0xff4fd1c5).withAlpha (0.35f), 0.0f, area.getY(),
                                   juce::Colour (0xffff6b6b).withAlpha (0.10f), 0.0f, area.getBottom(), false);
    g.setGradientFill (fillGrad);
    g.fillPath (fill);

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Band nodes
    for (int b = 0; b < eqs::numBands; ++b)
    {
        const auto p = nodePosition (b);
        const auto c = bandColours[b];
        const bool active = (b == draggedBand);

        g.setColour (c.withAlpha (0.25f));
        g.fillEllipse (p.x - 12.0f, p.y - 12.0f, 24.0f, 24.0f);

        g.setColour (c);
        g.fillEllipse (p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f);

        if (active)
        {
            g.setColour (juce::Colours::white);
            g.drawEllipse (p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f, 1.5f);
        }

        const juce::String label = (b == eqs::lowCut) ? juce::String ("LC")
                                 : (b == eqs::highShelf) ? juce::String ("HS")
                                 : juce::String (b);

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (label,
                    juce::Rectangle<float> (p.x - 12.0f, p.y - 26.0f, 24.0f, 12.0f),
                    juce::Justification::centred, false);
    }

    // Bottom control panel
    auto panel = juce::Rectangle<float> (0.0f, 380.0f, (float) getWidth(), (float) getHeight() - 380.0f);
    g.setColour (juce::Colour (0xff161a22));
    g.fillRect (panel);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (380, 0.0f, (float) getWidth());
}

void EQSceneEditor::resized()
{
    auto controls = getLocalBounds().withTrimmedTop (386).reduced (10, 6);
    const int colW = controls.getWidth() / (eqs::numBands + 1);

    for (int b = 0; b < eqs::numBands; ++b)
    {
        auto& ui = bandUI[(size_t) b];
        auto col = controls.removeFromLeft (colW).reduced (4, 0);

        ui.title.setBounds (col.removeFromTop (20));

        if (b == eqs::lowCut)
        {
            ui.sFreq.setBounds (col.reduced (4));
        }
        else
        {
            const int h = col.getHeight() / 3;
            ui.sFreq.setBounds (col.removeFromTop (h).reduced (2));
            ui.sGain.setBounds (col.removeFromTop (h).reduced (2));
            ui.sQ.setBounds (col.reduced (2));
        }
    }

    auto outCol = controls.removeFromLeft (colW).reduced (4, 0);
    outTitle.setBounds (outCol.removeFromTop (20));
    sOut.setBounds (outCol.reduced (4));
}

void EQSceneEditor::mouseDown (const juce::MouseEvent& e)
{
    draggedBand = findNodeAt (e.position);
    if (draggedBand >= 0)
        beginGesture (draggedBand, true);
    repaint();
}

void EQSceneEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBand < 0)
        return;

    const auto r = graphBounds();
    const int b = draggedBand;

    const float freq = std::clamp (xToFreq (e.position.x, r), minFreq, maxFreq);
    setParamValue (eqs::pid (b, "freq"), freq);

    if (b != eqs::lowCut)
    {
        const float gain = std::clamp (yToGain (e.position.y, r), -maxDb, maxDb);
        setParamValue (eqs::pid (b, "gain"), gain);
    }

    repaint();
}

void EQSceneEditor::mouseUp (const juce::MouseEvent&)
{
    if (draggedBand >= 0)
        beginGesture (draggedBand, false);

    draggedBand = -1;
    repaint();
}

void EQSceneEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const int b = findNodeAt (e.position);
    if (b < 0 || b == eqs::lowCut)
        return;

    const auto id = eqs::pid (b, "q");
    const float q = getParam (id);
    const float factor = std::pow (2.0f, wheel.deltaY * 2.0f);
    setParamValue (id, std::clamp (q * factor, 0.1f, 10.0f));
    repaint();
}
