#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

struct PitchResult
{
    float pitchHz = 0.0f;
    float confidence = 0.0f;
};

class PitchDetector
{
public:
    PitchDetector();

    void prepare (double sampleRate, int maxBlockSize);
    PitchResult detectPitch (const float* audioData, int numSamples);
    void setThreshold (float t) { yinThreshold = t; }

    // Lock-free reads for GUI thread
    float getDetectedPitch() const { return atomicPitch.load (std::memory_order_relaxed); }
    float getDetectedConfidence() const { return atomicConfidence.load (std::memory_order_relaxed); }

private:
    static constexpr int kWindowSize = 2048;
    static constexpr int kBufferSize = 3072; // W + W/2 for YIN
    static constexpr float kMinFreq = 60.0f;
    static constexpr float kMaxFreq = 1200.0f;
    static constexpr float kSilenceThreshold = 0.01f;

    double currentSampleRate = 44100.0;
    float yinThreshold = 0.15f;

    std::vector<float> ringBuffer;
    int writePos = 0;
    int samplesAccumulated = 0;

    std::vector<float> yinBuffer; // W/2 + 1 elements for difference function
    std::vector<float> analysisWindow; // Pre-allocated contiguous analysis buffer

    PitchResult latestResult;
    std::atomic<float> atomicPitch { 0.0f };
    std::atomic<float> atomicConfidence { 0.0f };

    PitchResult analyzeYIN();
    float computeRMS (const float* data, int numSamples);
};
