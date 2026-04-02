#include "PitchShifter.h"
#include <algorithm>

PitchShifter::PitchShifter()
{
    delayBuffer.resize (kBufferSize, 0.0f);
}

void PitchShifter::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;
    reset();
}

void PitchShifter::reset()
{
    std::fill (delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writePos = 0;

    // Start read taps behind write position
    int halfBuffer = kBufferSize / 2;
    readPos1 = 0.0;
    readPos2 = (double) halfBuffer;
    crossfadeMix = 0.0f;
    tap2Active = false;
    samplesSinceCrossfade = 0;
    crossfadeLength = kBufferSize / 8; // ~23ms at 44.1k
}

float PitchShifter::readFromBuffer (double pos) const
{
    // Linear interpolation from circular buffer
    int idx0 = ((int) pos) & kBufferMask;
    int idx1 = (idx0 + 1) & kBufferMask;
    float frac = (float) (pos - std::floor (pos));

    return delayBuffer[(size_t) idx0] * (1.0f - frac) + delayBuffer[(size_t) idx1] * frac;
}

float PitchShifter::processSample (float inputSample, float pitchRatio)
{
    // Write input to delay buffer
    delayBuffer[(size_t) writePos] = inputSample;
    writePos = (writePos + 1) & kBufferMask;

    // Advance read positions at modified rate
    // pitchRatio > 1.0 = read faster = pitch up
    // pitchRatio < 1.0 = read slower = pitch down
    readPos1 += (double) pitchRatio;
    readPos2 += (double) pitchRatio;

    // Keep read positions in valid range
    if (readPos1 >= (double) kBufferSize) readPos1 -= (double) kBufferSize;
    if (readPos1 < 0.0) readPos1 += (double) kBufferSize;
    if (readPos2 >= (double) kBufferSize) readPos2 -= (double) kBufferSize;
    if (readPos2 < 0.0) readPos2 += (double) kBufferSize;

    // Read from both taps
    float tap1 = readFromBuffer (readPos1);
    float tap2 = readFromBuffer (readPos2);

    // Check if primary tap is getting too close to write head
    // or too far behind — need to crossfade to secondary tap
    double dist1 = (double) writePos - readPos1;
    if (dist1 < 0.0) dist1 += (double) kBufferSize;

    double dist2 = (double) writePos - readPos2;
    if (dist2 < 0.0) dist2 += (double) kBufferSize;

    // Danger zone: too close to write head or too far (more than buffer)
    double minSafe = (double) crossfadeLength * 2.0;
    double maxSafe = (double) (kBufferSize - crossfadeLength * 2);

    bool tap1InDanger = (dist1 < minSafe || dist1 > maxSafe);

    if (tap1InDanger && !tap2Active)
    {
        // Start crossfade: place tap2 at a safe position (half buffer behind write)
        readPos2 = (double) writePos - (double) (kBufferSize / 2);
        if (readPos2 < 0.0) readPos2 += (double) kBufferSize;
        tap2Active = true;
        samplesSinceCrossfade = 0;
    }

    if (tap2Active)
    {
        ++samplesSinceCrossfade;
        crossfadeMix = std::clamp ((float) samplesSinceCrossfade / (float) crossfadeLength, 0.0f, 1.0f);

        if (crossfadeMix >= 1.0f)
        {
            // Crossfade complete: swap taps
            readPos1 = readPos2;
            tap2Active = false;
            crossfadeMix = 0.0f;
            samplesSinceCrossfade = 0;
            return tap2;
        }

        // Equal-power crossfade for smooth transition
        float gain1 = std::cos (crossfadeMix * juce::MathConstants<float>::halfPi);
        float gain2 = std::sin (crossfadeMix * juce::MathConstants<float>::halfPi);
        return tap1 * gain1 + tap2 * gain2;
    }

    return tap1;
}
