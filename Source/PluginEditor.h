#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TuneBoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TuneBoxLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
};

class TuneBoxAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit TuneBoxAudioProcessorEditor (TuneBoxAudioProcessor&);
    ~TuneBoxAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void paintPitchDisplay (juce::Graphics& g, juce::Rectangle<int> area);

    TuneBoxAudioProcessor& audioProcessor;
    TuneBoxLookAndFeel tuneLookAndFeel;

    // Colors
    static constexpr uint32_t kBgDark     = 0xFF1A1A2E;
    static constexpr uint32_t kPanelDark  = 0xFF16213E;
    static constexpr uint32_t kAccentRed  = 0xFFE94560;
    static constexpr uint32_t kAccentGold = 0xFFF5C518;
    static constexpr uint32_t kTextLight  = 0xFFEAEAEA;
    static constexpr uint32_t kTextDim    = 0xFF8899AA;
    static constexpr uint32_t kMeterGreen = 0xFF00E676;
    static constexpr uint32_t kMeterRed   = 0xFFFF1744;

    // Knobs
    juce::Slider retuneSpeedSlider;
    juce::Slider mixSlider;
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider sensitivitySlider;

    // Combo boxes
    juce::ComboBox keyCombo;
    juce::ComboBox scaleCombo;

    // Labels
    juce::Label retuneLabel, mixLabel, inputGainLabel, outputGainLabel, sensitivityLabel;
    juce::Label keyLabel, scaleLabel;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> retuneSpeedAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sensitivityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAttachment;

    // Cached pitch info for display
    float displayPitchHz = 0.0f;
    float displayConfidence = 0.0f;
    float displayTargetHz = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneBoxAudioProcessorEditor)
};
