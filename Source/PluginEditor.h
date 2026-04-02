/*
  ==============================================================================

    PluginEditor.h - FAUNA Audio Streaming Plugin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FAUNAAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    FAUNAAudioProcessorEditor (FAUNAAudioProcessor&);
    ~FAUNAAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void drawQRCode(juce::Graphics& g, const juce::String& url, int x, int y, int size);
    
    FAUNAAudioProcessor& audioProcessor;
    
    juce::Label urlLabel;
    
    juce::Label sampleRateLabel;
    juce::Label bufferSizeLabel;
    juce::Label portLabel;
    juce::Label statusLabel;
    juce::Label devicesLabel;
    
    bool qrCodeValid = false;
    juce::String lastURL;
    
    juce::Font titleFont;
    juce::Font urlFont;
    juce::Font labelFont;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FAUNAAudioProcessorEditor)
};
