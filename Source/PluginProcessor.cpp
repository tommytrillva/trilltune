#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Scale definitions (pitch classes 0-11 relative to root)
static const std::vector<int> scaleChromatic     = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const std::vector<int> scaleMajor         = { 0, 2, 4, 5, 7, 9, 11 };
static const std::vector<int> scaleNaturalMinor  = { 0, 2, 3, 5, 7, 8, 10 };
static const std::vector<int> scaleMajorPent     = { 0, 2, 4, 7, 9 };
static const std::vector<int> scaleMinorPent     = { 0, 3, 5, 7, 10 };
static const std::vector<int> scaleBlues         = { 0, 3, 5, 6, 7, 10 };
static const std::vector<int> scaleDorian        = { 0, 2, 3, 5, 7, 9, 10 };
static const std::vector<int> scaleMixolydian    = { 0, 2, 4, 5, 7, 9, 10 };
static const std::vector<int> scaleHarmonicMinor = { 0, 2, 3, 5, 7, 8, 11 };

const std::vector<int>& TuneBoxAudioProcessor::getScaleIntervals (int scaleIndex)
{
    switch (scaleIndex)
    {
        case 0:  return scaleChromatic;
        case 1:  return scaleMajor;
        case 2:  return scaleNaturalMinor;
        case 3:  return scaleMajorPent;
        case 4:  return scaleMinorPent;
        case 5:  return scaleBlues;
        case 6:  return scaleDorian;
        case 7:  return scaleMixolydian;
        case 8:  return scaleHarmonicMinor;
        default: return scaleChromatic;
    }
}

//==============================================================================
TuneBoxAudioProcessor::TuneBoxAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                        .withOutput ("Output", juce::AudioChannelSet::mono(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

TuneBoxAudioProcessor::~TuneBoxAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout TuneBoxAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "retuneSpeed", 1 }, "Retune Speed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "key", 1 }, "Key", 0, 11, 0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "scale", 1 }, "Scale", 0, 8, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 }, "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sensitivity", 1 }, "Sensitivity",
        juce::NormalisableRange<float> (0.05f, 0.50f, 0.01f), 0.15f));

    return { params.begin(), params.end() };
}

//==============================================================================
void TuneBoxAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    pitchDetector.prepare (sampleRate, samplesPerBlock);
    pitchShifter.prepare (sampleRate, samplesPerBlock);
    smoothedTargetPitch = 0.0f;
    firstDetection = true;
    // Pre-allocate generously — DAWs may pass blocks larger than samplesPerBlock
    dryBuffer.resize (std::max ((size_t) samplesPerBlock * 2, (size_t) 8192), 0.0f);
    setLatencySamples (512);
}

void TuneBoxAudioProcessor::releaseResources() {}

bool TuneBoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput  = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    // Mono in -> Mono out
    if (mainInput == juce::AudioChannelSet::mono() && mainOutput == juce::AudioChannelSet::mono())
        return true;

    // Stereo in -> Stereo out
    if (mainInput == juce::AudioChannelSet::stereo() && mainOutput == juce::AudioChannelSet::stereo())
        return true;

    return false;
}

//==============================================================================
float TuneBoxAudioProcessor::midiToFreq (float midiNote)
{
    return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
}

float TuneBoxAudioProcessor::freqToMidi (float hz)
{
    if (hz <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * std::log2 (hz / 440.0f);
}

float TuneBoxAudioProcessor::quantizePitchToScale (float pitchHz, int key, int scaleIndex)
{
    if (pitchHz <= 0.0f) return 0.0f;

    const auto& intervals = getScaleIntervals (scaleIndex);
    float midiFloat = freqToMidi (pitchHz);
    int midiRounded = (int) std::round (midiFloat);

    // Find the closest note in the scale
    float bestDist = 1000.0f;
    int bestMidi = midiRounded;

    // Search +/- 12 semitones to find the closest scale note
    for (int offset = -12; offset <= 12; ++offset)
    {
        int candidateMidi = midiRounded + offset;
        if (candidateMidi < 0 || candidateMidi > 127) continue;

        // Get pitch class relative to the key root
        int pitchClass = ((candidateMidi % 12) - key + 12) % 12;

        // Check if this pitch class is in the scale
        bool inScale = false;
        for (int interval : intervals)
        {
            if (pitchClass == interval)
            {
                inScale = true;
                break;
            }
        }

        if (inScale)
        {
            float dist = std::abs (midiFloat - (float) candidateMidi);
            if (dist < bestDist)
            {
                bestDist = dist;
                bestMidi = candidateMidi;
            }
        }
    }

    return midiToFreq ((float) bestMidi);
}

juce::String TuneBoxAudioProcessor::getNoteNameFromHz (float hz)
{
    if (hz <= 0.0f) return "---";

    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    float midi = freqToMidi (hz);
    int midiRounded = std::clamp ((int) std::round (midi), 0, 127);
    int note = midiRounded % 12;
    int octave = (midiRounded / 12) - 1;

    return juce::String (noteNames[note]) + juce::String (octave);
}

float TuneBoxAudioProcessor::getCentsOffset (float detectedHz, float targetHz)
{
    if (detectedHz <= 0.0f || targetHz <= 0.0f) return 0.0f;
    return 1200.0f * std::log2 (detectedHz / targetHz);
}

//==============================================================================
void TuneBoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    if (numSamples == 0) return;

    // Read parameters
    float retuneSpeed = apvts.getRawParameterValue ("retuneSpeed")->load();
    float mix         = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    int   key         = (int) apvts.getRawParameterValue ("key")->load();
    int   scale       = (int) apvts.getRawParameterValue ("scale")->load();
    float inputGainDb = apvts.getRawParameterValue ("inputGain")->load();
    float outputGainDb= apvts.getRawParameterValue ("outputGain")->load();
    float sensitivity = apvts.getRawParameterValue ("sensitivity")->load();

    float inputGain  = juce::Decibels::decibelsToGain (inputGainDb);
    float outputGain = juce::Decibels::decibelsToGain (outputGainDb);

    // Update pitch detector sensitivity
    pitchDetector.setThreshold (sensitivity);

    // Process only channel 0 (for stereo, we'll copy to channel 1 after)
    float* channelData = buffer.getWritePointer (0);

    // Apply input gain
    for (int i = 0; i < numSamples; ++i)
        channelData[i] *= inputGain;

    // Store dry signal (buffer pre-allocated in prepareToPlay)
    if ((size_t) numSamples > dryBuffer.size())
        return; // Safety bail — should never happen with proper prepareToPlay
    std::copy (channelData, channelData + numSamples, dryBuffer.begin());

    // Pitch detection
    PitchResult pitchResult = pitchDetector.detectPitch (channelData, numSamples);

    if (pitchResult.pitchHz > 0.0f && pitchResult.confidence > 0.2f)
    {
        // Quantize to scale
        float target = quantizePitchToScale (pitchResult.pitchHz, key, scale);

        // Retune speed smoothing
        float blockTimeSec = (float) numSamples / (float) currentSampleRate;
        float timeConstantSec = (retuneSpeed / 100.0f) * 0.5f;
        float smoothCoeff = 1.0f - std::exp (-blockTimeSec / std::max (timeConstantSec, 0.001f));

        if (firstDetection || smoothedTargetPitch <= 0.0f)
        {
            smoothedTargetPitch = target;
            firstDetection = false;
        }
        else
        {
            smoothedTargetPitch += (target - smoothedTargetPitch) * smoothCoeff;
        }

        // Compute pitch ratio
        float pitchRatio = smoothedTargetPitch / pitchResult.pitchHz;
        pitchRatio = std::clamp (pitchRatio, 0.5f, 2.0f);

        // Update grain size based on detected pitch
        pitchShifter.setGrainSizeFromPitch (pitchResult.pitchHz);

        // Pitch shift
        pitchShifter.process (channelData, numSamples, pitchRatio, pitchResult.confidence);

        // Update atomics for GUI
        detectedPitchHz.store (pitchResult.pitchHz, std::memory_order_relaxed);
        detectedConfidence.store (pitchResult.confidence, std::memory_order_relaxed);
        targetPitchHz.store (smoothedTargetPitch, std::memory_order_relaxed);
    }
    else
    {
        // Unvoiced: pass dry signal, update GUI
        detectedPitchHz.store (0.0f, std::memory_order_relaxed);
        detectedConfidence.store (0.0f, std::memory_order_relaxed);
        targetPitchHz.store (0.0f, std::memory_order_relaxed);

        // Still run through shifter with ratio 1.0 to maintain buffer state
        pitchShifter.process (channelData, numSamples, 1.0f, 0.0f);
    }

    // Dry/Wet mix
    for (int i = 0; i < numSamples; ++i)
        channelData[i] = dryBuffer[(size_t) i] * (1.0f - mix) + channelData[i] * mix;

    // Apply output gain
    for (int i = 0; i < numSamples; ++i)
        channelData[i] *= outputGain;

    // For stereo: copy channel 0 to channel 1
    if (totalNumOutputChannels >= 2 && buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }
}

//==============================================================================
juce::AudioProcessorEditor* TuneBoxAudioProcessor::createEditor()
{
    return new TuneBoxAudioProcessorEditor (*this);
}

void TuneBoxAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TuneBoxAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TuneBoxAudioProcessor();
}
