#include "PluginEditor.h"

//==============================================================================
// Custom LookAndFeel
//==============================================================================
TuneBoxLookAndFeel::TuneBoxLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF16213E));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xFFEAEAEA));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF8899AA));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xFFE94560));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF16213E));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xFFEAEAEA));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFFE94560));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xFFEAEAEA));
}

void TuneBoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour (juce::Colour (0xFF0D1527));
    g.fillEllipse (rx, ry, rw, rw);

    // Border
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.3f));
    g.drawEllipse (rx, ry, rw, rw, 1.5f);

    // Background arc
    juce::Path bgArc;
    bgArc.addCentredArc (centreX, centreY, radius - 4.0f, radius - 4.0f,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.15f));
    g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Active arc
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centreX, centreY, radius - 4.0f, radius - 4.0f,
                                0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (0xFFE94560));
        g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    // Pointer dot
    juce::Path pointer;
    auto pointerLength = radius - 8.0f;
    auto dotRadius = 3.0f;
    float dotX = centreX + pointerLength * std::cos (angle - juce::MathConstants<float>::halfPi);
    float dotY = centreY + pointerLength * std::sin (angle - juce::MathConstants<float>::halfPi);
    g.setColour (juce::Colour (0xFFEAEAEA));
    g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

//==============================================================================
// Editor
//==============================================================================
TuneBoxAudioProcessorEditor::TuneBoxAudioProcessorEditor (TuneBoxAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&tuneLookAndFeel);
    setSize (520, 520);

    // Helper to set up rotary sliders
    auto setupSlider = [this] (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible (slider);
    };

    auto setupLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colour (kTextDim));
        label.setFont (juce::Font (12.0f));
        addAndMakeVisible (label);
    };

    // Retune Speed (hero knob)
    setupSlider (retuneSpeedSlider);
    setupLabel (retuneLabel, "RETUNE SPEED");

    // Mix
    setupSlider (mixSlider);
    setupLabel (mixLabel, "MIX");

    // Input Gain
    setupSlider (inputGainSlider);
    setupLabel (inputGainLabel, "INPUT");

    // Output Gain
    setupSlider (outputGainSlider);
    setupLabel (outputGainLabel, "OUTPUT");

    // Sensitivity
    setupSlider (sensitivitySlider);
    setupLabel (sensitivityLabel, "SENSITIVITY");

    // Key combo
    keyCombo.addItem ("C",  1);
    keyCombo.addItem ("C#", 2);
    keyCombo.addItem ("D",  3);
    keyCombo.addItem ("D#", 4);
    keyCombo.addItem ("E",  5);
    keyCombo.addItem ("F",  6);
    keyCombo.addItem ("F#", 7);
    keyCombo.addItem ("G",  8);
    keyCombo.addItem ("G#", 9);
    keyCombo.addItem ("A",  10);
    keyCombo.addItem ("A#", 11);
    keyCombo.addItem ("B",  12);
    addAndMakeVisible (keyCombo);
    setupLabel (keyLabel, "KEY");

    // Scale combo
    scaleCombo.addItem ("Chromatic",       1);
    scaleCombo.addItem ("Major",           2);
    scaleCombo.addItem ("Minor",           3);
    scaleCombo.addItem ("Major Pent",      4);
    scaleCombo.addItem ("Minor Pent",      5);
    scaleCombo.addItem ("Blues",           6);
    scaleCombo.addItem ("Dorian",          7);
    scaleCombo.addItem ("Mixolydian",      8);
    scaleCombo.addItem ("Harmonic Minor",  9);
    addAndMakeVisible (scaleCombo);
    setupLabel (scaleLabel, "SCALE");

    // APVTS attachments
    retuneSpeedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "retuneSpeed", retuneSpeedSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "mix", mixSlider);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);
    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "sensitivity", sensitivitySlider);
    keyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "key", keyCombo);
    scaleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "scale", scaleCombo);

    startTimerHz (30);
}

TuneBoxAudioProcessorEditor::~TuneBoxAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void TuneBoxAudioProcessorEditor::timerCallback()
{
    displayPitchHz = audioProcessor.detectedPitchHz.load (std::memory_order_relaxed);
    displayConfidence = audioProcessor.detectedConfidence.load (std::memory_order_relaxed);
    displayTargetHz = audioProcessor.targetPitchHz.load (std::memory_order_relaxed);
    repaint (0, 50, getWidth(), 120); // Only repaint pitch display area
}

//==============================================================================
void TuneBoxAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (kBgDark));

    auto bounds = getLocalBounds();

    // Header bar
    {
        auto headerArea = bounds.removeFromTop (50);
        g.setColour (juce::Colour (kPanelDark));
        g.fillRect (headerArea);

        g.setColour (juce::Colour (kTextLight));
        g.setFont (juce::Font (24.0f, juce::Font::bold));
        g.drawText ("TUNEBOX", headerArea.reduced (15, 0), juce::Justification::centredLeft);

        g.setColour (juce::Colour (kTextDim));
        g.setFont (juce::Font (11.0f));
        g.drawText ("by Tommy Trill AI", headerArea.reduced (15, 0), juce::Justification::centredRight);

        // Bottom border
        g.setColour (juce::Colour (kAccentRed).withAlpha (0.5f));
        g.drawLine ((float) headerArea.getX(), (float) headerArea.getBottom(),
                    (float) headerArea.getRight(), (float) headerArea.getBottom(), 1.0f);
    }

    // Pitch display section
    {
        auto pitchArea = bounds.toFloat().removeFromTop (120.0f).toNearestInt();
        paintPitchDisplay (g, pitchArea);
    }

    // Control section hints
    {
        // "HARD TUNE" and "NATURAL" labels around retune speed
        g.setColour (juce::Colour (kTextDim));
        g.setFont (juce::Font (9.0f));

        auto retuneArea = retuneSpeedSlider.getBounds();
        g.drawText ("<< HARD TUNE", retuneArea.getX() - 70, retuneArea.getBottom() - 5, 70, 15,
                    juce::Justification::centredRight);
        g.drawText ("NATURAL >>", retuneArea.getRight(), retuneArea.getBottom() - 5, 70, 15,
                    juce::Justification::centredLeft);

        // Value readouts under knobs
        g.setFont (juce::Font (10.0f));
        g.setColour (juce::Colour (kAccentGold));

        float retuneVal = retuneSpeedSlider.getValue();
        float timeMs = (retuneVal / 100.0f) * 500.0f;
        g.drawText (juce::String ((int) timeMs) + " ms", retuneArea.getX(), retuneArea.getBottom() + 2,
                    retuneArea.getWidth(), 14, juce::Justification::centred);

        auto mixArea = mixSlider.getBounds();
        g.drawText (juce::String ((int) mixSlider.getValue()) + "%", mixArea.getX(), mixArea.getBottom() + 2,
                    mixArea.getWidth(), 14, juce::Justification::centred);

        auto inArea = inputGainSlider.getBounds();
        g.drawText (juce::String (inputGainSlider.getValue(), 1) + " dB", inArea.getX(), inArea.getBottom() + 2,
                    inArea.getWidth(), 14, juce::Justification::centred);

        auto outArea = outputGainSlider.getBounds();
        g.drawText (juce::String (outputGainSlider.getValue(), 1) + " dB", outArea.getX(), outArea.getBottom() + 2,
                    outArea.getWidth(), 14, juce::Justification::centred);

        auto sensArea = sensitivitySlider.getBounds();
        g.drawText (juce::String (sensitivitySlider.getValue(), 2), sensArea.getX(), sensArea.getBottom() + 2,
                    sensArea.getWidth(), 14, juce::Justification::centred);
    }
}

void TuneBoxAudioProcessorEditor::paintPitchDisplay (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour (juce::Colour (kPanelDark).withAlpha (0.5f));
    g.fillRoundedRectangle (area.reduced (10).toFloat(), 6.0f);

    auto inner = area.reduced (15);

    // Detected note name (large)
    juce::String noteName = TuneBoxAudioProcessor::getNoteNameFromHz (displayPitchHz);
    g.setColour (juce::Colour (kTextLight));
    g.setFont (juce::Font (36.0f, juce::Font::bold));
    g.drawText (noteName, inner.removeFromTop (42), juce::Justification::centred);

    // Cents offset bar
    if (displayPitchHz > 0.0f && displayTargetHz > 0.0f)
    {
        auto centsArea = inner.removeFromTop (20).reduced (40, 2);
        float cents = TuneBoxAudioProcessor::getCentsOffset (displayPitchHz, displayTargetHz);
        cents = std::clamp (cents, -50.0f, 50.0f);

        // Background bar
        g.setColour (juce::Colour (kTextDim).withAlpha (0.2f));
        g.fillRoundedRectangle (centsArea.toFloat(), 3.0f);

        // Center marker
        float centerX = centsArea.toFloat().getCentreX();
        g.setColour (juce::Colour (kTextDim).withAlpha (0.4f));
        g.drawVerticalLine ((int) centerX, (float) centsArea.getY(), (float) centsArea.getBottom());

        // Indicator dot
        float dotX = centerX + (cents / 50.0f) * (centsArea.getWidth() / 2.0f);
        juce::Colour dotColour;
        float absCents = std::abs (cents);
        if (absCents <= 10.0f)
            dotColour = juce::Colour (kMeterGreen);
        else if (absCents <= 25.0f)
            dotColour = juce::Colour (kAccentGold);
        else
            dotColour = juce::Colour (kMeterRed);

        g.setColour (dotColour);
        g.fillEllipse (dotX - 5.0f, centsArea.getCentreY() - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        inner.removeFromTop (20);
    }

    // Hz readout
    g.setColour (juce::Colour (kTextDim));
    g.setFont (juce::Font (11.0f));
    juce::String hzText;
    if (displayPitchHz > 0.0f)
        hzText = juce::String (displayPitchHz, 1) + " Hz -> " + juce::String (displayTargetHz, 1) + " Hz";
    else
        hzText = "--- Hz";
    g.drawText (hzText, inner.removeFromTop (16), juce::Justification::centred);

    // Confidence bar
    auto confArea = inner.removeFromTop (12).reduced (60, 2);
    g.setColour (juce::Colour (kTextDim).withAlpha (0.2f));
    g.fillRoundedRectangle (confArea.toFloat(), 2.0f);

    if (displayConfidence > 0.0f)
    {
        auto fillWidth = confArea.getWidth() * displayConfidence;
        juce::Colour confColour = displayConfidence > 0.5f ? juce::Colour (kMeterGreen) : juce::Colour (kAccentGold);
        g.setColour (confColour);
        g.fillRoundedRectangle (confArea.toFloat().withWidth (fillWidth), 2.0f);
    }

    g.setColour (juce::Colour (kTextDim));
    g.setFont (juce::Font (9.0f));
    g.drawText ("CONFIDENCE", confArea.translated (0, 10), juce::Justification::centred);
}

void TuneBoxAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (50);  // header
    bounds.removeFromTop (120); // pitch display

    // Key/Scale row
    {
        auto row = bounds.removeFromTop (55);
        auto inner = row.reduced (30, 5);

        auto leftHalf = inner.removeFromLeft (inner.getWidth() / 2).reduced (10, 0);
        auto rightHalf = inner.reduced (10, 0);

        keyLabel.setBounds (leftHalf.removeFromTop (15));
        keyCombo.setBounds (leftHalf);

        scaleLabel.setBounds (rightHalf.removeFromTop (15));
        scaleCombo.setBounds (rightHalf);
    }

    // Main controls
    {
        auto controlArea = bounds.reduced (10);

        // Top row: Input Gain | RETUNE SPEED (large) | Output Gain
        auto topRow = controlArea.removeFromTop (140);

        int sideKnobW = 70;
        int heroKnobW = 130;

        auto leftKnob = topRow.removeFromLeft (sideKnobW + 20);
        auto rightKnob = topRow.removeFromRight (sideKnobW + 20);

        // Center the hero knob
        auto heroArea = topRow;
        int heroX = heroArea.getCentreX() - heroKnobW / 2;
        int heroY = heroArea.getY() + 5;

        inputGainLabel.setBounds (leftKnob.getX() + 10, leftKnob.getY(), sideKnobW, 15);
        inputGainSlider.setBounds (leftKnob.getX() + 10, leftKnob.getY() + 15, sideKnobW, sideKnobW);

        retuneLabel.setBounds (heroX, heroY, heroKnobW, 15);
        retuneSpeedSlider.setBounds (heroX, heroY + 15, heroKnobW, heroKnobW);

        outputGainLabel.setBounds (rightKnob.getX() + 10, rightKnob.getY(), sideKnobW, 15);
        outputGainSlider.setBounds (rightKnob.getX() + 10, rightKnob.getY() + 15, sideKnobW, sideKnobW);

        // Bottom row: Mix | Sensitivity
        auto bottomRow = controlArea.removeFromTop (100);
        int medKnobW = 80;

        int spacing = 60;
        int totalW = medKnobW * 2 + spacing;
        int startX = bottomRow.getCentreX() - totalW / 2;
        int yPos = bottomRow.getY() + 5;

        mixLabel.setBounds (startX, yPos, medKnobW, 15);
        mixSlider.setBounds (startX, yPos + 15, medKnobW, medKnobW);

        sensitivityLabel.setBounds (startX + medKnobW + spacing, yPos, medKnobW, 15);
        sensitivitySlider.setBounds (startX + medKnobW + spacing, yPos + 15, medKnobW, medKnobW);
    }
}
