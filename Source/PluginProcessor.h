#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "PitchShifter.h"
#include <atomic>

class TuneBoxAudioProcessor : public juce::AudioProcessor
{
public:
    TuneBoxAudioProcessor();
    ~TuneBoxAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
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

    // Atomic values for GUI display
    std::atomic<float> detectedPitchHz { 0.0f };
    std::atomic<float> detectedConfidence { 0.0f };
    std::atomic<float> targetPitchHz { 0.0f };

    // Helper: get note name from frequency
    static juce::String getNoteNameFromHz (float hz);
    static float getCentsOffset (float detectedHz, float targetHz);

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Scale definitions: pitch classes relative to root
    static constexpr int kNumScales = 9;
    static const std::vector<int>& getScaleIntervals (int scaleIndex);

    // Scale quantizer
    float quantizePitchToScale (float pitchHz, int key, int scaleIndex);
    static float midiToFreq (float midiNote);
    static float freqToMidi (float hz);

    PitchDetector pitchDetector;
    PitchShifter pitchShifter;

    float smoothedTargetPitch = 0.0f;
    float currentPitchRatio = 1.0f;
    float lastDetectedPitch = 0.0f;
    float lastConfidence = 0.0f;
    bool firstDetection = true;
    double currentSampleRate = 44100.0;

    std::vector<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneBoxAudioProcessor)
};
