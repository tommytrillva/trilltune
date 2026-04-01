#include "PluginEditor.h"

//==============================================================================
// Custom LookAndFeel
//==============================================================================
TuneBoxLookAndFeel::TuneBoxLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xFF16213E));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xFFEAEAEA));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xFF8899AA).withAlpha (0.3f));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xFFE94560));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xFF16213E));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xFFEAEAEA));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xFFE94560));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xFFEAEAEA));
}

void TuneBoxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    bool isHeroKnob = (radius > 40.0f); // Hero knob is larger
    auto accentColour = isHeroKnob ? juce::Colour (0xFFF5C518) : juce::Colour (0xFFE94560);
    float arcThickness = isHeroKnob ? 7.0f : 5.0f;

    // Outer shadow / glow
    {
        juce::ColourGradient shadow (accentColour.withAlpha (0.08f), centreX, centreY,
                                     accentColour.withAlpha (0.0f), centreX, centreY - radius - 12.0f, true);
        g.setGradientFill (shadow);
        g.fillEllipse (rx - 6.0f, ry - 6.0f, rw + 12.0f, rw + 12.0f);
    }

    // Background circle with gradient
    {
        juce::ColourGradient bgGrad (juce::Colour (0xFF151B30), centreX, ry,
                                      juce::Colour (0xFF0A0F1E), centreX, ry + rw, false);
        g.setGradientFill (bgGrad);
        g.fillEllipse (rx, ry, rw, rw);
    }

    // Border ring
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.15f));
    g.drawEllipse (rx, ry, rw, rw, 1.0f);

    // Inner shadow circle
    g.setColour (juce::Colour (0xFF0A0F1E));
    g.fillEllipse (rx + 8.0f, ry + 8.0f, rw - 16.0f, rw - 16.0f);
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.08f));
    g.drawEllipse (rx + 8.0f, ry + 8.0f, rw - 16.0f, rw - 16.0f, 0.5f);

    // Background arc track
    {
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, radius - 5.0f, radius - 5.0f,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.1f));
        g.strokePath (bgArc, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Active arc with glow
    if (sliderPos > 0.005f)
    {
        // Glow layer (wider, transparent)
        {
            juce::Path glowArc;
            glowArc.addCentredArc (centreX, centreY, radius - 5.0f, radius - 5.0f,
                                    0.0f, rotaryStartAngle, angle, true);
            g.setColour (accentColour.withAlpha (0.15f));
            g.strokePath (glowArc, juce::PathStrokeType (arcThickness + 6.0f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        }

        // Main arc
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centreX, centreY, radius - 5.0f, radius - 5.0f,
                                    0.0f, rotaryStartAngle, angle, true);
            g.setColour (accentColour);
            g.strokePath (valueArc, juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
        }
    }

    // Pointer dot with glow
    {
        auto pointerLength = radius - 10.0f;
        float dotR = isHeroKnob ? 5.0f : 4.0f;
        float dotX = centreX + pointerLength * std::cos (angle - juce::MathConstants<float>::halfPi);
        float dotY = centreY + pointerLength * std::sin (angle - juce::MathConstants<float>::halfPi);

        // Dot glow
        g.setColour (accentColour.withAlpha (0.3f));
        g.fillEllipse (dotX - dotR - 2.0f, dotY - dotR - 2.0f, (dotR + 2.0f) * 2.0f, (dotR + 2.0f) * 2.0f);

        // Dot
        g.setColour (juce::Colour (0xFFEAEAEA));
        g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);
    }
}

void TuneBoxLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                        int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                        juce::ComboBox& /*box*/)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();

    // Background with subtle gradient
    juce::ColourGradient bg (juce::Colour (0xFF1C2951), 0.0f, 0.0f,
                              juce::Colour (0xFF16213E), 0.0f, (float) height, false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, 6.0f);

    // Border
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.2f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Arrow
    auto arrowArea = bounds.removeFromRight (25.0f).reduced (8.0f, 10.0f);
    juce::Path arrow;
    arrow.addTriangle (arrowArea.getX(), arrowArea.getY(),
                       arrowArea.getRight(), arrowArea.getY(),
                       arrowArea.getCentreX(), arrowArea.getBottom());
    g.setColour (juce::Colour (0xFFE94560));
    g.fillPath (arrow);
}

void TuneBoxLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (0xFF16213E));
    g.setColour (juce::Colour (0xFF8899AA).withAlpha (0.2f));
    g.drawRect (0, 0, width, height, 1);
}

void TuneBoxLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool /*isSeparator*/, bool isActive, bool isHighlighted,
                                             bool isTicked, bool /*hasSubMenu*/,
                                             const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                             const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isHighlighted && isActive)
    {
        g.setColour (juce::Colour (0xFFE94560));
        g.fillRect (area);
    }

    g.setColour (isHighlighted ? juce::Colour (0xFFEAEAEA) : juce::Colour (0xFFCCCCCC));
    g.setFont (juce::Font (13.0f));
    g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft);

    if (isTicked)
    {
        g.setColour (juce::Colour (0xFFF5C518));
        g.fillEllipse ((float) (area.getRight() - 16), (float) area.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
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
        label.setFont (juce::Font (11.0f, juce::Font::bold));
        addAndMakeVisible (label);
    };

    setupSlider (retuneSpeedSlider);
    setupLabel (retuneLabel, "RETUNE SPEED");

    setupSlider (mixSlider);
    setupLabel (mixLabel, "MIX");

    setupSlider (inputGainSlider);
    setupLabel (inputGainLabel, "INPUT");

    setupSlider (outputGainSlider);
    setupLabel (outputGainLabel, "OUTPUT");

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
    keyCombo.setJustificationType (juce::Justification::centred);
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
    scaleCombo.setJustificationType (juce::Justification::centred);
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
    float rawPitch = audioProcessor.detectedPitchHz.load (std::memory_order_relaxed);
    float rawConf  = audioProcessor.detectedConfidence.load (std::memory_order_relaxed);
    displayTargetHz = audioProcessor.targetPitchHz.load (std::memory_order_relaxed);

    // Smooth the display values for animation (lerp at ~30Hz)
    const float smoothing = 0.3f;
    if (rawPitch > 0.0f)
    {
        if (smoothedDisplayPitch <= 0.0f)
            smoothedDisplayPitch = rawPitch; // snap on first detection
        else
            smoothedDisplayPitch += (rawPitch - smoothedDisplayPitch) * smoothing;
    }
    else
    {
        smoothedDisplayPitch = 0.0f;
    }
    smoothedDisplayConfidence += (rawConf - smoothedDisplayConfidence) * smoothing;

    displayPitchHz = smoothedDisplayPitch;
    displayConfidence = smoothedDisplayConfidence;

    repaint (0, 50, getWidth(), 130);
}

//==============================================================================
void TuneBoxAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background gradient
    {
        juce::ColourGradient bg (juce::Colour (kBgMid), 0.0f, 0.0f,
                                  juce::Colour (kBgDark), 0.0f, (float) getHeight(), false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    auto bounds = getLocalBounds();

    // Header
    paintHeader (g, bounds.removeFromTop (50));

    // Pitch display
    paintPitchDisplay (g, bounds.removeFromTop (130));

    // Control section background
    paintControlBackground (g, bounds);

    // Value readouts under knobs
    {
        g.setFont (juce::Font (10.0f, juce::Font::bold));

        // Retune Speed
        auto retuneArea = retuneSpeedSlider.getBounds();
        float retuneVal = retuneSpeedSlider.getValue();
        float timeMs = (retuneVal / 100.0f) * 500.0f;
        g.setColour (juce::Colour (kAccentGold));
        g.drawText (juce::String ((int) timeMs) + " ms", retuneArea.getX(), retuneArea.getBottom() + 2,
                    retuneArea.getWidth(), 14, juce::Justification::centred);

        // "HARD TUNE" / "NATURAL" hints
        g.setColour (juce::Colour (kTextDim).withAlpha (0.6f));
        g.setFont (juce::Font (8.0f, juce::Font::bold));
        g.drawText ("HARD", retuneArea.getX() - 50, retuneArea.getBottom() + 2, 45, 12,
                    juce::Justification::centredRight);
        g.drawText ("NATURAL", retuneArea.getRight() + 5, retuneArea.getBottom() + 2, 50, 12,
                    juce::Justification::centredLeft);

        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colour (kAccentRed));

        // Mix
        auto mixArea = mixSlider.getBounds();
        g.drawText (juce::String ((int) mixSlider.getValue()) + "%", mixArea.getX(), mixArea.getBottom() + 2,
                    mixArea.getWidth(), 14, juce::Justification::centred);

        // Input Gain
        auto inArea = inputGainSlider.getBounds();
        g.drawText (juce::String (inputGainSlider.getValue(), 1) + " dB", inArea.getX(), inArea.getBottom() + 2,
                    inArea.getWidth(), 14, juce::Justification::centred);

        // Output Gain
        auto outArea = outputGainSlider.getBounds();
        g.drawText (juce::String (outputGainSlider.getValue(), 1) + " dB", outArea.getX(), outArea.getBottom() + 2,
                    outArea.getWidth(), 14, juce::Justification::centred);

        // Sensitivity
        auto sensArea = sensitivitySlider.getBounds();
        g.drawText (juce::String (sensitivitySlider.getValue(), 2), sensArea.getX(), sensArea.getBottom() + 2,
                    sensArea.getWidth(), 14, juce::Justification::centred);
    }
}

void TuneBoxAudioProcessorEditor::paintHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Header background with gradient
    {
        juce::ColourGradient hdrBg (juce::Colour (kPanelLight), 0.0f, (float) area.getY(),
                                     juce::Colour (kPanelDark), 0.0f, (float) area.getBottom(), false);
        g.setGradientFill (hdrBg);
        g.fillRect (area);
    }

    // Title "TUNEBOX" with slight letter spacing feel
    g.setColour (juce::Colour (kTextLight));
    g.setFont (juce::Font (26.0f, juce::Font::bold));
    g.drawText ("TUNEBOX", area.reduced (18, 0), juce::Justification::centredLeft);

    // Accent dot after title
    g.setColour (juce::Colour (kAccentRed));
    g.fillEllipse (108.0f, (float) area.getCentreY() - 3.0f, 6.0f, 6.0f);

    // Subtitle
    g.setColour (juce::Colour (kTextDim).withAlpha (0.7f));
    g.setFont (juce::Font (10.0f));
    g.drawText ("by Tommy Trill AI", area.reduced (18, 0), juce::Justification::centredRight);

    // Bottom accent line with glow
    float lineY = (float) area.getBottom() - 1.0f;
    {
        juce::ColourGradient lineGlow (juce::Colour (kAccentRed).withAlpha (0.0f), (float) area.getX(), lineY,
                                        juce::Colour (kAccentRed).withAlpha (0.8f), (float) area.getCentreX(), lineY, false);
        lineGlow.addColour (1.0, juce::Colour (kAccentRed).withAlpha (0.0f));
        g.setGradientFill (lineGlow);
        g.fillRect ((float) area.getX(), lineY - 1.0f, (float) area.getWidth(), 2.0f);
    }

    // Subtle glow under the line
    {
        juce::ColourGradient underGlow (juce::Colour (kAccentRed).withAlpha (0.12f), (float) area.getCentreX(), lineY,
                                         juce::Colour (kAccentRed).withAlpha (0.0f), (float) area.getCentreX(), lineY + 10.0f, false);
        g.setGradientFill (underGlow);
        g.fillRect ((float) area.getX() + 50.0f, lineY, (float) area.getWidth() - 100.0f, 10.0f);
    }
}

void TuneBoxAudioProcessorEditor::paintPitchDisplay (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto panelArea = area.reduced (12, 6);

    // Panel background with subtle gradient
    {
        juce::ColourGradient panelBg (juce::Colour (kPanelDark).withAlpha (0.7f),
                                       (float) panelArea.getX(), (float) panelArea.getY(),
                                       juce::Colour (kBgDark).withAlpha (0.5f),
                                       (float) panelArea.getX(), (float) panelArea.getBottom(), false);
        g.setGradientFill (panelBg);
        g.fillRoundedRectangle (panelArea.toFloat(), 8.0f);
    }

    // Panel border
    g.setColour (juce::Colour (kTextDim).withAlpha (0.1f));
    g.drawRoundedRectangle (panelArea.toFloat(), 8.0f, 1.0f);

    auto inner = panelArea.reduced (15, 8);

    // Detected note name (large, with color based on confidence)
    juce::String noteName = TuneBoxAudioProcessor::getNoteNameFromHz (displayPitchHz);
    if (displayConfidence > 0.5f)
        g.setColour (juce::Colour (kTextLight));
    else if (displayConfidence > 0.2f)
        g.setColour (juce::Colour (kAccentGold));
    else
        g.setColour (juce::Colour (kTextDim));
    g.setFont (juce::Font (40.0f, juce::Font::bold));
    g.drawText (noteName, inner.removeFromTop (46), juce::Justification::centred);

    // Cents offset bar
    {
        auto centsArea = inner.removeFromTop (22).reduced (35, 3);

        // Background bar
        g.setColour (juce::Colour (kTextDim).withAlpha (0.12f));
        g.fillRoundedRectangle (centsArea.toFloat(), 4.0f);

        if (displayPitchHz > 0.0f && displayTargetHz > 0.0f)
        {
            float cents = TuneBoxAudioProcessor::getCentsOffset (displayPitchHz, displayTargetHz);
            cents = std::clamp (cents, -50.0f, 50.0f);
            float centerX = centsArea.toFloat().getCentreX();

            // Center tick marks
            g.setColour (juce::Colour (kTextDim).withAlpha (0.25f));
            g.drawVerticalLine ((int) centerX, (float) centsArea.getY() + 2.0f, (float) centsArea.getBottom() - 2.0f);
            // Quarter marks
            float qw = centsArea.getWidth() / 4.0f;
            for (int q = 1; q <= 3; ++q)
            {
                g.setColour (juce::Colour (kTextDim).withAlpha (0.1f));
                g.drawVerticalLine ((int) (centsArea.getX() + qw * q), (float) centsArea.getY() + 4.0f,
                                   (float) centsArea.getBottom() - 4.0f);
            }

            // Indicator dot with color and glow
            float dotX = centerX + (cents / 50.0f) * (centsArea.getWidth() / 2.0f);
            float absCents = std::abs (cents);
            juce::Colour dotColour;
            if (absCents <= 10.0f)
                dotColour = juce::Colour (kMeterGreen);
            else if (absCents <= 25.0f)
                dotColour = juce::Colour (kAccentGold);
            else
                dotColour = juce::Colour (kMeterRed);

            // Glow
            g.setColour (dotColour.withAlpha (0.2f));
            g.fillEllipse (dotX - 8.0f, centsArea.getCentreY() - 8.0f, 16.0f, 16.0f);

            // Dot
            g.setColour (dotColour);
            g.fillEllipse (dotX - 5.0f, centsArea.getCentreY() - 5.0f, 10.0f, 10.0f);

            // Bright center
            g.setColour (dotColour.brighter (0.3f));
            g.fillEllipse (dotX - 2.5f, centsArea.getCentreY() - 2.5f, 5.0f, 5.0f);
        }
    }

    // Hz readout
    {
        auto hzArea = inner.removeFromTop (16);
        g.setFont (juce::Font (11.0f));
        if (displayPitchHz > 0.0f)
        {
            // Detected Hz in dim
            g.setColour (juce::Colour (kTextDim));
            juce::String hzText = juce::String (displayPitchHz, 1) + " Hz";
            g.drawText (hzText, hzArea.removeFromLeft (hzArea.getWidth() / 2), juce::Justification::centredRight);

            // Arrow
            g.setColour (juce::Colour (kAccentRed).withAlpha (0.6f));
            g.drawText (" > ", hzArea.removeFromLeft (20), juce::Justification::centred);

            // Target Hz in brighter
            g.setColour (juce::Colour (kTextLight).withAlpha (0.8f));
            g.drawText (juce::String (displayTargetHz, 1) + " Hz", hzArea, juce::Justification::centredLeft);
        }
        else
        {
            g.setColour (juce::Colour (kTextDim).withAlpha (0.5f));
            g.drawText ("- - -", hzArea, juce::Justification::centred);
        }
    }

    // Confidence bar
    {
        auto confRow = inner.removeFromTop (18);
        auto confArea = confRow.reduced (55, 4);

        // Label
        g.setColour (juce::Colour (kTextDim).withAlpha (0.5f));
        g.setFont (juce::Font (8.0f, juce::Font::bold));
        g.drawText ("CONFIDENCE", confArea.translated (0, 10), juce::Justification::centred);

        // Bar background
        g.setColour (juce::Colour (kTextDim).withAlpha (0.1f));
        g.fillRoundedRectangle (confArea.toFloat(), 3.0f);

        if (displayConfidence > 0.0f)
        {
            float fillWidth = confArea.getWidth() * std::clamp (displayConfidence, 0.0f, 1.0f);

            juce::Colour confColour;
            if (displayConfidence > 0.6f)
                confColour = juce::Colour (kMeterGreen);
            else if (displayConfidence > 0.3f)
                confColour = juce::Colour (kAccentGold);
            else
                confColour = juce::Colour (kMeterRed);

            // Glow behind bar
            g.setColour (confColour.withAlpha (0.15f));
            g.fillRoundedRectangle (confArea.toFloat().withWidth (fillWidth).expanded (0.0f, 3.0f), 4.0f);

            // Bar fill
            g.setColour (confColour);
            g.fillRoundedRectangle (confArea.toFloat().withWidth (fillWidth), 3.0f);
        }
    }
}

void TuneBoxAudioProcessorEditor::paintControlBackground (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Subtle separator line below key/scale row
    auto sepY = area.getY() + 55;
    g.setColour (juce::Colour (kTextDim).withAlpha (0.08f));
    g.drawHorizontalLine (sepY, (float) area.getX() + 30.0f, (float) area.getRight() - 30.0f);

    // Panel behind the main knobs
    auto knobPanel = area.reduced (15, 60).withTop (sepY + 5);
    {
        juce::ColourGradient panelBg (juce::Colour (kPanelDark).withAlpha (0.3f),
                                       (float) knobPanel.getCentreX(), (float) knobPanel.getY(),
                                       juce::Colour (kBgDark).withAlpha (0.1f),
                                       (float) knobPanel.getCentreX(), (float) knobPanel.getBottom(), false);
        g.setGradientFill (panelBg);
        g.fillRoundedRectangle (knobPanel.toFloat(), 10.0f);
    }
    g.setColour (juce::Colour (kTextDim).withAlpha (0.06f));
    g.drawRoundedRectangle (knobPanel.toFloat(), 10.0f, 1.0f);
}

void TuneBoxAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (50);  // header
    bounds.removeFromTop (130); // pitch display

    // Key/Scale row
    {
        auto row = bounds.removeFromTop (55);
        auto inner = row.reduced (40, 8);

        auto leftHalf = inner.removeFromLeft (inner.getWidth() / 2).reduced (8, 0);
        auto rightHalf = inner.reduced (8, 0);

        keyLabel.setBounds (leftHalf.removeFromTop (14));
        keyCombo.setBounds (leftHalf.reduced (0, 2));

        scaleLabel.setBounds (rightHalf.removeFromTop (14));
        scaleCombo.setBounds (rightHalf.reduced (0, 2));
    }

    // Main controls
    {
        auto controlArea = bounds.reduced (10);

        // Top row: Input Gain | RETUNE SPEED (large) | Output Gain
        auto topRow = controlArea.removeFromTop (150);

        int sideKnobW = 70;
        int heroKnobW = 140;

        auto leftKnob = topRow.removeFromLeft (sideKnobW + 20);
        auto rightKnob = topRow.removeFromRight (sideKnobW + 20);

        auto heroArea = topRow;
        int heroX = heroArea.getCentreX() - heroKnobW / 2;
        int heroY = heroArea.getY() + 5;

        inputGainLabel.setBounds (leftKnob.getX() + 10, leftKnob.getY() + 8, sideKnobW, 14);
        inputGainSlider.setBounds (leftKnob.getX() + 10, leftKnob.getY() + 24, sideKnobW, sideKnobW);

        retuneLabel.setBounds (heroX, heroY, heroKnobW, 14);
        retuneSpeedSlider.setBounds (heroX, heroY + 16, heroKnobW, heroKnobW);

        outputGainLabel.setBounds (rightKnob.getX() + 10, rightKnob.getY() + 8, sideKnobW, 14);
        outputGainSlider.setBounds (rightKnob.getX() + 10, rightKnob.getY() + 24, sideKnobW, sideKnobW);

        // Bottom row: Mix | Sensitivity
        auto bottomRow = controlArea.removeFromTop (100);
        int medKnobW = 80;

        int spacing = 60;
        int totalW = medKnobW * 2 + spacing;
        int startX = bottomRow.getCentreX() - totalW / 2;
        int yPos = bottomRow.getY();

        mixLabel.setBounds (startX, yPos, medKnobW, 14);
        mixSlider.setBounds (startX, yPos + 16, medKnobW, medKnobW);

        sensitivityLabel.setBounds (startX + medKnobW + spacing, yPos, medKnobW, 14);
        sensitivitySlider.setBounds (startX + medKnobW + spacing, yPos + 16, medKnobW, medKnobW);
    }
}
