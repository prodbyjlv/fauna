/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 350);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FAUNA", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Server: Starting...", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(14.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(ipLabel);
    ipLabel.setText("IP: --", juce::dontSendNotification);
    ipLabel.setFont(juce::FontOptions(16.0f));
    ipLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    ipLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(urlLabel);
    urlLabel.setText("http://--.--.--:8080", juce::dontSendNotification);
    urlLabel.setFont(juce::FontOptions(12.0f));
    urlLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    urlLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(clientsLabel);
    clientsLabel.setText("Connected: 0 devices", juce::dontSendNotification);
    clientsLabel.setFont(juce::FontOptions(12.0f));
    clientsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    clientsLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(infoLabel);
    infoLabel.setText("Enter URL in mobile browser to control audio", juce::dontSendNotification);
    infoLabel.setFont(juce::FontOptions(11.0f));
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::darkgrey);
    infoLabel.setJustificationType(juce::Justification::centred);
    
    startTimer(500);
}

FAUNAAudioProcessorEditor::~FAUNAAudioProcessorEditor()
{
    stopTimer();
}

void FAUNAAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkblue.darker());
    
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
}

void FAUNAAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(20, 20);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    statusLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(20);
    ipLabel.setBounds(bounds.removeFromTop(30));
    urlLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(20);
    clientsLabel.setBounds(bounds.removeFromTop(25));
    infoLabel.setBounds(bounds.removeFromTop(25));
}

void FAUNAAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.isServerRunning())
    {
        statusLabel.setText("Server: Running", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
        
        juce::String ip = audioProcessor.httpServer.getLocalIPAddress();
        ipLabel.setText("IP: " + ip, juce::dontSendNotification);
        urlLabel.setText("http://" + ip + ":8080", juce::dontSendNotification);
        
        clientsLabel.setText("Connected: " + juce::String(audioProcessor.getConnectedClients()) + " device(s)", 
                           juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Server: Stopped", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}
