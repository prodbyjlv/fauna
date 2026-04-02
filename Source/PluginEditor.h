/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class QRCodeGenerator
{
public:
    static juce::Image generateQRCode(const juce::String& data, int size = 200);
};

class FAUNAAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    FAUNAAudioProcessorEditor (FAUNAAudioProcessor&);
    ~FAUNAAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    FAUNAAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label ipLabel;
    juce::Label urlLabel;
    juce::Label clientsLabel;
    juce::Label infoLabel;
    juce::ImageComponent qrImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FAUNAAudioProcessorEditor)
};
