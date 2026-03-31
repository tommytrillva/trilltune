#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

// With 4x overlap Hann windows, the sum of windows is approximately 2.0.
// We divide by this to normalize the output level.
static constexpr float kHannOlaNormalization = 2.0f;

PitchShifter::PitchShifter()
{
}

void PitchShifter::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    // Input buffer: ~100ms + max grain size
    int hundredMs = (int) (sampleRate * 0.1);
    inputBufferSize = hundredMs + kMaxGrainSize;
    inputBuffer.assign ((size_t) inputBufferSize, 0.0f);
    inputWritePos = 0;

    // Output buffer: ~100ms + 2 * max grain size
    outputBufferSize = hundredMs + 2 * kMaxGrainSize;
    outputBuffer.assign ((size_t) outputBufferSize, 0.0f);
    outputReadPos = 0;
    // Start the write position ahead of read position by one grain size
    // so grains are fully written before we read them
    outputWritePos = kMaxGrainSize;

    grainSize = 512;
    nextGrainSize = 512;
    hopSize = grainSize / kOverlap;
    samplesSinceLastGrain = 0;

    // Pre-compute Hann window storage
    hannWindow.resize (kMaxGrainSize);
    currentWindow.resize (kMaxGrainSize);
    currentWindowSize = 0;
}

void PitchShifter::setGrainSizeFromPitch (float detectedPitchHz)
{
    // Buffer the grain size change — it will be applied at the next grain boundary
    if (detectedPitchHz > 0.0f)
    {
        int period = (int) (currentSampleRate / detectedPitchHz);
        nextGrainSize = std::clamp (period * 2, kMinGrainSize, kMaxGrainSize);
    }
    else
    {
        nextGrainSize = 512;
    }
}

void PitchShifter::computeHannWindow (int size)
{
    if (size == currentWindowSize)
        return;

    currentWindowSize = size;
    for (int i = 0; i < size; ++i)
        currentWindow[(size_t) i] = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) size));
}

float PitchShifter::readInputSample (double pos) const
{
    int idx0 = (int) std::floor (pos);
    float frac = (float) (pos - (double) idx0);

    idx0 = ((idx0 % inputBufferSize) + inputBufferSize) % inputBufferSize;
    int idx1 = (idx0 + 1) % inputBufferSize;

    return inputBuffer[(size_t) idx0] * (1.0f - frac) + inputBuffer[(size_t) idx1] * frac;
}

void PitchShifter::emitGrain (float pitchRatio)
{
    computeHannWindow (grainSize);

    // The grain reads from the input buffer starting at a position
    // behind the current write position
    double grainStart = (double) inputWritePos - (double) grainSize;

    for (int i = 0; i < grainSize; ++i)
    {
        // Resample: read at modified rate for pitch shifting
        double readIdx = grainStart + (double) i * (double) pitchRatio;
        float sample = readInputSample (readIdx);

        // Apply Hann window
        sample *= currentWindow[(size_t) i];

        // Accumulate into output buffer
        int outIdx = (outputWritePos + i) % outputBufferSize;
        outputBuffer[(size_t) outIdx] += sample;
    }

    outputWritePos = (outputWritePos + hopSize) % outputBufferSize;
}

void PitchShifter::process (float* audioData, int numSamples, float pitchRatio, float confidence)
{
    // Clamp pitch ratio
    pitchRatio = std::clamp (pitchRatio, kMinRatio, kMaxRatio);

    for (int i = 0; i < numSamples; ++i)
    {
        float drySample = audioData[i];

        // Write input sample to ring buffer
        inputBuffer[(size_t) inputWritePos] = drySample;
        inputWritePos = (inputWritePos + 1) % inputBufferSize;

        // Check if it's time to emit a new grain
        ++samplesSinceLastGrain;
        if (samplesSinceLastGrain >= hopSize)
        {
            samplesSinceLastGrain = 0;

            // Apply buffered grain size change at grain boundary
            grainSize = nextGrainSize;
            hopSize = grainSize / kOverlap;

            emitGrain (pitchRatio);
        }

        // Read from output accumulation buffer, normalized
        float shiftedSample = outputBuffer[(size_t) outputReadPos] / kHannOlaNormalization;
        outputBuffer[(size_t) outputReadPos] = 0.0f; // Clear after reading
        outputReadPos = (outputReadPos + 1) % outputBufferSize;

        // Confidence-based blending with dry signal
        if (confidence < kConfidenceThreshold)
        {
            float blend = confidence / kConfidenceThreshold;
            audioData[i] = shiftedSample * blend + drySample * (1.0f - blend);
        }
        else
        {
            audioData[i] = shiftedSample;
        }
    }
}
