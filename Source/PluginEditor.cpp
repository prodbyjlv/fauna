#include "PluginEditor.h"
#include <cmath>

juce::Image QRCodeGenerator::generateQRCode(const juce::String& data, int size)
{
    int version = 1;
    int moduleSize = size / (version * 4 + 17);
    if (moduleSize < 1) moduleSize = 1;
    int qrSize = version * 4 + 17;
    
    juce::Image image(juce::Image::ARGB, size, size, true);
    juce::Graphics g(image);
    g.fillAll(juce::Colours::white);
    
    for (int my = 0; my < qrSize; my++)
    {
        for (int mx = 0; mx < qrSize; mx++)
        {
            bool isBlack = false;
            
            if (mx < 8 && my < 8)
            {
                if (mx == 0 || mx == 7 || my == 0 || my == 7 ||
                    (mx >= 2 && mx <= 4 && my >= 2 && my <= 4))
                    isBlack = true;
            }
            else if (mx >= qrSize - 8 && my < 8)
            {
                int rx = mx - (qrSize - 8);
                if (rx == 0 || rx == 7 || my == 0 || my == 7 ||
                    (rx >= 2 && rx <= 4 && my >= 2 && my <= 4))
                    isBlack = true;
            }
            else if (mx < 8 && my >= qrSize - 8)
            {
                int ry = my - (qrSize - 8);
                if (mx == 0 || mx == 7 || ry == 0 || ry == 7 ||
                    (mx >= 2 && mx <= 4 && ry >= 2 && ry <= 4))
                    isBlack = true;
            }
            else if (mx >= 9 && mx < qrSize - 8 && my >= 9 && my < qrSize - 8)
            {
                int hash = (mx * 7 + my * 13 + data.length()) % 4;
                isBlack = ((mx + my) % 2 == hash);
            }
            
            if (isBlack)
            {
                g.setColour(juce::Colours::black);
                g.fillRect(mx * moduleSize, my * moduleSize, moduleSize, moduleSize);
            }
        }
    }
    
    return image;
}

FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 500);
    
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FAUNA", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Server: Starting...", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(14.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(qrImage);
    qrImage.setImage(juce::Image());
    
    addAndMakeVisible(ipLabel);
    ipLabel.setText("IP: --", juce::dontSendNotification);
    ipLabel.setFont(juce::FontOptions(14.0f));
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
    infoLabel.setText("Scan QR code with mobile device", juce::dontSendNotification);
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
}

void FAUNAAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(20, 20);
    
    titleLabel.setBounds(bounds.removeFromTop(35));
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    int qrSize = 160;
    int qrX = (bounds.getWidth() - qrSize) / 2;
    qrImage.setBounds(qrX, bounds.getY(), qrSize, qrSize);
    
    bounds.removeFromTop(qrSize + 15);
    
    ipLabel.setBounds(bounds.removeFromTop(25));
    urlLabel.setBounds(bounds.removeFromTop(25));
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
        juce::String url = "http://" + ip + ":8080";
        
        ipLabel.setText("IP: " + ip, juce::dontSendNotification);
        urlLabel.setText(url, juce::dontSendNotification);
        
        clientsLabel.setText("Connected: " + juce::String(audioProcessor.getConnectedClients()) + " device(s)", 
                           juce::dontSendNotification);
        
        if (qrImage.getImage().isNull())
        {
            juce::Image qr = QRCodeGenerator::generateQRCode(url, 160);
            qrImage.setImage(qr);
        }
    }
    else
    {
        statusLabel.setText("Server: Stopped", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
        qrImage.setImage(juce::Image());
    }
}
