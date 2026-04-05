#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "QrCode.hpp"

class FAUNAAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit FAUNAAudioProcessorEditor (FAUNAAudioProcessor&);
    ~FAUNAAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;

    void paintWatermark (juce::Graphics&) const;
    void paintMetaRow   (juce::Graphics&) const;
    void paintQRCode    (juce::Graphics&) const;
    void paintQRGrid    (juce::Graphics&, juce::Rectangle<float>) const;
    void paintFooter    (juce::Graphics&) const;

    static constexpr int W = 340;
    static constexpr int H = 400;

    const juce::Colour colBg    { 0xff1c1510 };
    const juce::Colour colCream { 0xfff0e6d0 };
    const juce::Colour colCoral { 0xffff5a46 };
    const juce::Colour colGreen { 0xff4ade80 };

    int          connectedDevices { 0 };
    juce::String serverIP         { "0.0.0.0" };
    int          serverPort       { 8080 };
    double       streamSampleRate { 44100.0 };

    FAUNAAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FAUNAAudioProcessorEditor)
};
