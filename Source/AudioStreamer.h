#pragma once

#include <JuceHeader.h>

class AudioStreamer
{
public:
    AudioStreamer();
    ~AudioStreamer();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);

    int getAvailableSamples();
    void consumeSamples(float* outputBuffer, int numSamples);

    float getCurrentLevel() const;
    float getCurrentLevelLeft() const;
    float getCurrentLevelRight() const;

private:
    juce::AudioBuffer<float> circularBuffer;
    int writePosition = 0;
    int readPosition = 0;
    float currentLevelLeft = 0.0f;
    float currentLevelRight = 0.0f;

    int bufferSize = 0;
    int sampleRate = 44100;

    JUCE_LEAK_DETECTOR(AudioStreamer)
};
