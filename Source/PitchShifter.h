#pragma once
#include <JuceHeader.h>
#include <vector>

class PitchShifter
{
public:
    PitchShifter();

    void prepare (double sampleRate, int maxBlockSize);
    void process (float* audioData, int numSamples, float pitchRatio, float confidence);
    void setGrainSizeFromPitch (float detectedPitchHz);

private:
    static constexpr int kMaxGrainSize = 2048;
    static constexpr int kMinGrainSize = 128;
    static constexpr int kOverlap = 4;
    static constexpr float kMinRatio = 0.5f;
    static constexpr float kMaxRatio = 2.0f;
    static constexpr float kConfidenceThreshold = 0.2f;

    double currentSampleRate = 44100.0;

    // Input ring buffer
    std::vector<float> inputBuffer;
    int inputBufferSize = 0;
    int inputWritePos = 0;

    // Output accumulation ring buffer
    std::vector<float> outputBuffer;
    int outputBufferSize = 0;
    int outputReadPos = 0;
    int outputWritePos = 0;

    // Grain parameters
    int grainSize = 512;
    int hopSize = 128;
    int nextGrainSize = 512; // Buffered grain size, applied at grain boundary
    int samplesSinceLastGrain = 0;

    // Pre-computed Hann window (max size)
    std::vector<float> hannWindow;

    void emitGrain (float pitchRatio);
    float readInputSample (double pos) const;
    void computeHannWindow (int size);

    // Current Hann window cache
    std::vector<float> currentWindow;
    int currentWindowSize = 0;
};
