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
    currentLevelLeft = 0.0f;
    currentLevelRight = 0.0f;
}

void AudioStreamer::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float maxLevelLeft = 0.0f;
    float maxLevelRight = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            int writeIdx = writePosition;

            if (numChannels == 2)
            {
                float left = buffer.getSample(0, i);
                float right = buffer.getSample(1, i);
                circularBuffer.setSample(0, writeIdx, left);
                circularBuffer.setSample(1, writeIdx, right);
                
                float absLeft = std::abs(left);
                float absRight = std::abs(right);
                if (absLeft > maxLevelLeft) maxLevelLeft = absLeft;
                if (absRight > maxLevelRight) maxLevelRight = absRight;
            }
            else
            {
                circularBuffer.setSample(0, writeIdx, channelData[i]);
                circularBuffer.setSample(1, writeIdx, channelData[i]);
                
                float absSample = std::abs(channelData[i]);
                if (absSample > maxLevelLeft) maxLevelLeft = absSample;
                if (absSample > maxLevelRight) maxLevelRight = absSample;
            }

            writePosition = (writeIdx + 1) % bufferSize;
        }
    }

    currentLevelLeft = juce::jmax(maxLevelLeft, currentLevelLeft * 0.85f);
    currentLevelRight = juce::jmax(maxLevelRight, currentLevelRight * 0.85f);
}

int AudioStreamer::getAvailableSamples()
{
    int w = writePosition;
    int r = readPosition;

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
            int r = readPosition;
            outputBuffer[i * 2] = circularBuffer.getSample(0, r);
            outputBuffer[i * 2 + 1] = circularBuffer.getSample(1, r);
            readPosition = (r + 1) % bufferSize;
        }
        else
        {
            outputBuffer[i * 2] = 0.0f;
            outputBuffer[i * 2 + 1] = 0.0f;
        }
    }
}

float AudioStreamer::getCurrentLevel() const
{
    return (currentLevelLeft + currentLevelRight) * 0.5f;
}

float AudioStreamer::getCurrentLevelLeft() const
{
    return currentLevelLeft;
}

float AudioStreamer::getCurrentLevelRight() const
{
    return currentLevelRight;
}
