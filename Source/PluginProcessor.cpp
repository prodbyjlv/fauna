/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FAUNAAudioProcessor::FAUNAAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FAUNAAudioProcessor::~FAUNAAudioProcessor()
{
    httpServer.stop();
}

const juce::String FAUNAAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FAUNAAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FAUNAAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FAUNAAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FAUNAAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FAUNAAudioProcessor::getNumPrograms()
{
    return 1;
}

int FAUNAAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FAUNAAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FAUNAAudioProcessor::getProgramName (int index)
{
    return {};
}

void FAUNAAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void FAUNAAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    audioStreamer.prepareToPlay(sampleRate, samplesPerBlock);
    httpServer.start(8080);
}

void FAUNAAudioProcessor::releaseResources()
{
    httpServer.stop();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FAUNAAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FAUNAAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    audioStreamer.processBlock(buffer);
    httpServer.setLevel(audioStreamer.getCurrentLevel());
    httpServer.processConnections();
}

bool FAUNAAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FAUNAAudioProcessor::createEditor()
{
    return new FAUNAAudioProcessorEditor (*this);
}

void FAUNAAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void FAUNAAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FAUNAAudioProcessor();
}
