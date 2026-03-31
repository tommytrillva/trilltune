#include "PitchShifter.h"
#include <cmath>
#include <algorithm>

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
    outputWritePos = 0;

    grainSize = 512;
    hopSize = grainSize / kOverlap;
    samplesSinceLastGrain = 0;
    grainReadPos = 0.0;

    // Pre-compute max Hann window
    hannWindow.resize (kMaxGrainSize);
    for (int i = 0; i < kMaxGrainSize; ++i)
        hannWindow[(size_t) i] = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) kMaxGrainSize));

    currentWindow.resize (kMaxGrainSize);
    currentWindowSize = 0;
}

void PitchShifter::setGrainSizeFromPitch (float detectedPitchHz)
{
    if (detectedPitchHz > 0.0f)
    {
        // Grain size = 2 * pitch period
        int period = (int) (currentSampleRate / detectedPitchHz);
        grainSize = std::clamp (period * 2, kMinGrainSize, kMaxGrainSize);
    }
    else
    {
        grainSize = 512;
    }
    hopSize = grainSize / kOverlap;
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
            emitGrain (pitchRatio);
        }

        // Read from output accumulation buffer
        float shiftedSample = outputBuffer[(size_t) outputReadPos];
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
