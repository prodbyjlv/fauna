#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <mutex>

class AudioStreamer
{
public:
    AudioStreamer();
    ~AudioStreamer();

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);

    int getAvailableSamples();
    void consumeSamples(float* outputBuffer, int numSamples);

    float getCurrentLevel();

private:
    juce::AudioBuffer<float> circularBuffer;
    std::atomic<int> writePosition{ 0 };
    std::atomic<int> readPosition{ 0 };
    std::atomic<float> currentLevel{ 0.0f };

    int bufferSize = 0;
    int sampleRate = 44100;
    std::mutex bufferMutex;

    JUCE_LEAK_DETECTOR(AudioStreamer)
};
