#include "PluginEditor.h"

FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 420);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FAUNA", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Server: Starting...", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(14.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(ipLabel);
    ipLabel.setText("IP: --", juce::dontSendNotification);
    ipLabel.setFont(juce::FontOptions(14.0f));
    ipLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    ipLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(urlLabel);
    urlLabel.setText("Open this URL on your phone:", juce::dontSendNotification);
    urlLabel.setFont(juce::FontOptions(12.0f));
    urlLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    urlLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(urlBigLabel);
    urlBigLabel.setText("http://--.--.--:8080", juce::dontSendNotification);
    urlBigLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    urlBigLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    urlBigLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(clientsLabel);
    clientsLabel.setText("Connected: 0 devices", juce::dontSendNotification);
    clientsLabel.setFont(juce::FontOptions(14.0f));
    clientsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    clientsLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(infoLabel);
    infoLabel.setText("Type the URL above into your phone's browser", juce::dontSendNotification);
    infoLabel.setFont(juce::FontOptions(12.0f));
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    infoLabel.setJustificationType(juce::Justification::centred);
    
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
    bounds.reduce(20, 20);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    statusLabel.setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(20);
    
    ipLabel.setBounds(bounds.removeFromTop(30));
    urlLabel.setBounds(bounds.removeFromTop(25));
    urlBigLabel.setBounds(bounds.removeFromTop(45));
    
    clientsLabel.setBounds(bounds.removeFromTop(30));
    infoLabel.setBounds(bounds.removeFromTop(30));
}

void FAUNAAudioProcessorEditor::timerCallback()
{
    static bool ipDetected = false;
    static int attemptCount = 0;
    
    if (!audioProcessor.isServerRunning())
    {
        attemptCount++;
        if (attemptCount <= 10)
        {
            statusLabel.setText("Server: Starting... (" + juce::String(attemptCount) + ")", juce::dontSendNotification);
        }
        return;
    }
    
    if (!ipDetected)
    {
        audioProcessor.httpServer.detectLocalIP();
        ipDetected = true;
    }
    
    statusLabel.setText("Server: Running", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    
    juce::String ip = audioProcessor.httpServer.getLocalIPAddress();
    juce::String urlText = "http://" + ip + ":8080";
    
    ipLabel.setText("IP: " + ip, juce::dontSendNotification);
    urlBigLabel.setText(urlText, juce::dontSendNotification);
    
    juce::String clientText = "Connected: " + juce::String(audioProcessor.getConnectedClients()) + " device(s)";
    clientsLabel.setText(clientText, juce::dontSendNotification);
}
