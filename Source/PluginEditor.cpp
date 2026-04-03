#include "PluginEditor.h"

FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 500);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FAUNA", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(qrCode);
    
    addAndMakeVisible(scanLabel);
    scanLabel.setText("Scan to connect", juce::dontSendNotification);
    scanLabel.setFont(juce::FontOptions(12.0f));
    scanLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    scanLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(urlLabel);
    urlLabel.setText("http://--.--.--:8080", juce::dontSendNotification);
    urlLabel.setFont(juce::FontOptions(14.0f));
    urlLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    urlLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(infoLabel);
    infoLabel.setText("SR: --- Hz  |  Port: 8080", juce::dontSendNotification);
    infoLabel.setFont(juce::FontOptions(11.0f));
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    infoLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(connectedLabel);
    connectedLabel.setText("Connected: 0 devices", juce::dontSendNotification);
    connectedLabel.setFont(juce::FontOptions(11.0f));
    connectedLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    connectedLabel.setJustificationType(juce::Justification::centred);
    
    startTimer(200);
}

FAUNAAudioProcessorEditor::~FAUNAAudioProcessorEditor()
{
    stopTimer();
}

void FAUNAAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkblue.darker());
}

void FAUNAAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    int w = bounds.getWidth();
    
    titleLabel.setBounds(bounds.removeFromTop(45));
    
    int qrSize = juce::jmin(w - 80, 250);
    int qrX = (w - qrSize) / 2;
    int qrY = bounds.getY() + 20;
    qrCode.setBounds(qrX, qrY, qrSize, qrSize);
    
    int labelWidth = w - 40;
    int labelX = 20;
    int currentY = qrY + qrSize + 15;
    
    scanLabel.setBounds(labelX, currentY, labelWidth, 20);
    currentY += 25;
    
    urlLabel.setBounds(labelX, currentY, labelWidth, 22);
    currentY += 28;
    
    infoLabel.setBounds(labelX, currentY, labelWidth, 18);
    currentY += 24;
    
    connectedLabel.setBounds(labelX, currentY, labelWidth, 18);
}

void FAUNAAudioProcessorEditor::timerCallback()
{
    static bool ipDetected = false;
    static int attemptCount = 0;
    static juce::String lastUrl = "";

    if (!audioProcessor.isServerRunning())
    {
        attemptCount++;
        if (attemptCount <= 10)
            urlLabel.setText("Starting... (" + juce::String(attemptCount) + ")", juce::dontSendNotification);
        return;
    }

    if (!ipDetected)
    {
        audioProcessor.httpServer.detectLocalIP();
        ipDetected = true;
    }

    juce::String ip = audioProcessor.httpServer.getLocalIPAddress();
    juce::String urlText = "http://" + ip + ":8080";

    urlLabel.setText(urlText, juce::dontSendNotification);

    // Only regenerate QR code if URL has changed — avoids wasteful redraws every 200ms
    if (urlText != lastUrl)
    {
        qrCode.generate(urlText);
        lastUrl = urlText;
    }

    juce::String clientText = "Connected: " + juce::String(audioProcessor.getConnectedClients()) + " device(s)";
    connectedLabel.setText(clientText, juce::dontSendNotification);
}
