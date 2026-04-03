/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "QrCode.hpp"

class QRCodeComponent : public juce::Component
{
public:
    QRCodeComponent() {}

    void generate(const juce::String& url)
    {
        if (url.isEmpty())
        {
            qrImage = juce::Image();
            return;
        }

        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url.toUTF8(), qrcodegen::QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        int scale = 6;
        qrImage = juce::Image(juce::Image::ARGB, size * scale, size * scale, true);
        juce::Graphics g(qrImage);

        g.fillAll(juce::Colours::white);

        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                if (qr.getModule(x, y))
                {
                    g.setColour(juce::Colours::black);
                    g.fillRect(x * scale, y * scale, scale, scale);
                }
            }
        }
    }

    void paint(juce::Graphics& g) override
    {
        if (qrImage.isValid())
        {
            g.drawImage(qrImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
        }
    }

private:
    juce::Image qrImage;
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
    juce::Label urlLabel;
    juce::Label scanLabel;
    juce::Label infoLabel;
    juce::Label statusLabel;
    juce::Label connectedLabel;
    QRCodeComponent qrCode;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FAUNAAudioProcessorEditor)
};
