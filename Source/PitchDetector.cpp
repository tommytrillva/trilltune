#include "PitchDetector.h"
#include <cmath>
#include <algorithm>

PitchDetector::PitchDetector()
{
    ringBuffer.resize (kBufferSize, 0.0f);
    yinBuffer.resize (kWindowSize / 2 + 1, 0.0f);
}

void PitchDetector::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;
    writePos = 0;
    samplesAccumulated = 0;
    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    latestResult = {};
    atomicPitch.store (0.0f, std::memory_order_relaxed);
    atomicConfidence.store (0.0f, std::memory_order_relaxed);
}

PitchResult PitchDetector::detectPitch (const float* audioData, int numSamples)
{
    // Push samples into ring buffer
    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[(size_t) writePos] = audioData[i];
        writePos = (writePos + 1) % kBufferSize;
        ++samplesAccumulated;
    }

    // Only analyze when we have enough samples
    if (samplesAccumulated >= kWindowSize)
    {
        samplesAccumulated = 0;
        latestResult = analyzeYIN();
        atomicPitch.store (latestResult.pitchHz, std::memory_order_relaxed);
        atomicConfidence.store (latestResult.confidence, std::memory_order_relaxed);
    }

    return latestResult;
}

float PitchDetector::computeRMS (const float* data, int numSamples)
{
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        sum += data[i] * data[i];
    return std::sqrt (sum / (float) numSamples);
}

PitchResult PitchDetector::analyzeYIN()
{
    const int halfWindow = kWindowSize / 2;

    // Extract the analysis window from ring buffer into a contiguous array
    std::vector<float> window (kBufferSize);
    for (int i = 0; i < kBufferSize; ++i)
        window[(size_t) i] = ringBuffer[(size_t) ((writePos - kBufferSize + i + kBufferSize * 2) % kBufferSize)];

    // Silence gate: check RMS of the most recent window
    float rms = computeRMS (window.data() + (kBufferSize - kWindowSize), kWindowSize);
    if (rms < kSilenceThreshold)
        return { 0.0f, 0.0f };

    // Step 1: Difference function
    // d(tau) = sum_{j=0}^{W/2-1} (x[j] - x[j+tau])^2
    // We read from the tail of the window buffer
    const float* x = window.data() + (kBufferSize - kWindowSize - halfWindow);

    yinBuffer[0] = 0.0f;
    for (int tau = 1; tau <= halfWindow; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < halfWindow; ++j)
        {
            float delta = x[j] - x[j + tau];
            sum += delta * delta;
        }
        yinBuffer[(size_t) tau] = sum;
    }

    // Step 2: Cumulative mean normalized difference
    // d'(0) = 1, d'(tau) = d(tau) / ((1/tau) * sum_{j=1}^{tau} d(j))
    yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;
    for (int tau = 1; tau <= halfWindow; ++tau)
    {
        runningSum += yinBuffer[(size_t) tau];
        if (runningSum > 0.0f)
            yinBuffer[(size_t) tau] = yinBuffer[(size_t) tau] * (float) tau / runningSum;
        else
            yinBuffer[(size_t) tau] = 1.0f;
    }

    // Step 3: Absolute threshold search
    int tauMin = std::max (1, (int) (currentSampleRate / kMaxFreq));
    int tauMax = std::min (halfWindow, (int) (currentSampleRate / kMinFreq));

    int bestTau = -1;
    for (int tau = tauMin; tau < tauMax; ++tau)
    {
        if (yinBuffer[(size_t) tau] < yinThreshold)
        {
            // Find local minimum in this valley
            while (tau + 1 < tauMax && yinBuffer[(size_t) (tau + 1)] < yinBuffer[(size_t) tau])
                ++tau;
            bestTau = tau;
            break;
        }
    }

    // If no tau found below threshold, find the global minimum
    if (bestTau < 0)
    {
        float minVal = 1.0f;
        bestTau = tauMin;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (yinBuffer[(size_t) tau] < minVal)
            {
                minVal = yinBuffer[(size_t) tau];
                bestTau = tau;
            }
        }
        // If global minimum is still too high, return unvoiced
        if (minVal > 0.5f)
            return { 0.0f, 0.0f };
    }

    // Step 4: Parabolic interpolation for sub-sample accuracy
    float betterTau = (float) bestTau;
    if (bestTau > 0 && bestTau < halfWindow)
    {
        float s0 = yinBuffer[(size_t) (bestTau - 1)];
        float s1 = yinBuffer[(size_t) bestTau];
        float s2 = yinBuffer[(size_t) (bestTau + 1)];
        float denom = 2.0f * (s0 - 2.0f * s1 + s2);
        if (std::abs (denom) > 1e-12f)
            betterTau = (float) bestTau + (s0 - s2) / denom;
    }

    // Compute pitch and confidence
    float pitchHz = (float) (currentSampleRate / (double) betterTau);
    float confidence = 1.0f - yinBuffer[(size_t) bestTau];
    confidence = std::clamp (confidence, 0.0f, 1.0f);

    // Frequency range check
    if (pitchHz < kMinFreq || pitchHz > kMaxFreq)
        return { 0.0f, 0.0f };

    return { pitchHz, confidence };
}
