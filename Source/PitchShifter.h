#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class PitchShifter
{
public:
    PitchShifter();

    void prepare (double sampleRate, int maxBlockSize);
    float processSample (float inputSample, float pitchRatio);
    void reset();

private:
    static constexpr int kBufferSize = 8192; // Power of 2 for fast masking
    static constexpr int kBufferMask = kBufferSize - 1;
    static constexpr float kCrossfadeRegion = 0.5f; // Proportion of buffer for crossfade

    double currentSampleRate = 44100.0;

    // Circular delay buffer
    std::vector<float> delayBuffer;
    int writePos = 0;

    // Two read taps that crossfade to hide discontinuities
    double readPos1 = 0.0;
    double readPos2 = 0.0;
    float crossfadeMix = 0.0f; // 0.0 = tap1 only, 1.0 = tap2 only

    // Crossfade state
    bool tap2Active = false;
    int samplesSinceCrossfade = 0;
    int crossfadeLength = 512;

    float readFromBuffer (double pos) const;
};
