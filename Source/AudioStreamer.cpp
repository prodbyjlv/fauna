#include "AudioStreamer.h"
#include <cmath>

AudioStreamer::AudioStreamer()
{
}

AudioStreamer::~AudioStreamer()
{
}

void AudioStreamer::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    this->sampleRate = (int)sampleRate;
    bufferSize = maximumExpectedSamplesPerBlock * 100;

    circularBuffer.setSize(2, bufferSize);
    circularBuffer.clear();

    writePosition = 0;
    readPosition = 0;
}

void AudioStreamer::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float maxLevel = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            int writeIdx = writePosition.load();

            if (numChannels == 2)
            {
                float left = buffer.getSample(0, i);
                float right = buffer.getSample(1, i);
                circularBuffer.setSample(0, writeIdx, left);
                circularBuffer.setSample(1, writeIdx, right);
            }
            else
            {
                circularBuffer.setSample(0, writeIdx, channelData[i]);
                circularBuffer.setSample(1, writeIdx, channelData[i]);
            }

            float sample = std::abs(channelData[i]);
            if (sample > maxLevel)
                maxLevel = sample;

            writePosition.store((writeIdx + 1) % bufferSize);
        }
    }

    currentLevel.store(maxLevel);
}

int AudioStreamer::getAvailableSamples()
{
    int w = writePosition.load();
    int r = readPosition.load();

    if (w >= r)
        return w - r;
    else
        return bufferSize - r + w;
}

void AudioStreamer::consumeSamples(float* outputBuffer, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        if (getAvailableSamples() > 0)
        {
            int r = readPosition.load();
            outputBuffer[i * 2] = circularBuffer.getSample(0, r);
            outputBuffer[i * 2 + 1] = circularBuffer.getSample(1, r);
            readPosition.store((r + 1) % bufferSize);
        }
        else
        {
            outputBuffer[i * 2] = 0.0f;
            outputBuffer[i * 2 + 1] = 0.0f;
        }
    }
}

float AudioStreamer::getCurrentLevel()
{
    return currentLevel.load();
}
