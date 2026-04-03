/*
  ==============================================================================
    PluginProcessor.cpp - FAUNA Audio Streaming Plugin
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FAUNAAudioProcessor::FAUNAAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output",  juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FAUNAAudioProcessor::~FAUNAAudioProcessor()
{
    httpServer.stop();
}

const juce::String FAUNAAudioProcessor::getName() const { return JucePlugin_Name; }
bool FAUNAAudioProcessor::acceptsMidi() const { return false; }
bool FAUNAAudioProcessor::producesMidi() const { return false; }
bool FAUNAAudioProcessor::isMidiEffect() const { return false; }
double FAUNAAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int FAUNAAudioProcessor::getNumPrograms() { return 1; }
int FAUNAAudioProcessor::getCurrentProgram() { return 0; }
void FAUNAAudioProcessor::setCurrentProgram(int) {}
const juce::String FAUNAAudioProcessor::getProgramName(int) { return {}; }
void FAUNAAudioProcessor::changeProgramName(int, const juce::String&) {}

void FAUNAAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    httpServer.start(8080);
}

void FAUNAAudioProcessor::releaseResources()
{
    httpServer.stop();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FAUNAAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FAUNAAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that don't have input data
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Interleave stereo audio into a flat float array [L0,R0, L1,R1, ...]
    // This is exactly what the browser's Float32Array expects
    juce::HeapBlock<float> interleaved(numSamples * 2);
    float* out = interleaved.get();

    for (int i = 0; i < numSamples; ++i)
    {
        float left  = buffer.getSample(0, i);
        float right = (numChannels > 1) ? buffer.getSample(1, i) : left;

        // Clamp to [-1, 1] to prevent WebSocket payload corruption
        left  = juce::jlimit(-1.0f, 1.0f, left);
        right = juce::jlimit(-1.0f, 1.0f, right);

        out[i * 2]     = left;
        out[i * 2 + 1] = right;
    }

    // Send to all connected WebSocket clients
    // numSamples is per-channel count; broadcastAudio expects per-channel count
    httpServer.writeAudioData(out, numSamples);
}

bool FAUNAAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* FAUNAAudioProcessor::createEditor()
{
    return new FAUNAAudioProcessorEditor(*this);
}

void FAUNAAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void FAUNAAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FAUNAAudioProcessor();
}
