#include "PluginEditor.h"
#include "QrCode.hpp"

FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (450, 420);
    
    titleFont = juce::FontOptions(64.0f, juce::Font::bold);
    urlFont = juce::FontOptions(16.0f, juce::Font::bold);
    labelFont = juce::FontOptions(11.0f);
    
    addAndMakeVisible(urlLabel);
    urlLabel.setText("http://--.--.--.--:8080", juce::dontSendNotification);
    urlLabel.setFont(urlFont);
    urlLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFD4AF00));
    urlLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(sampleRateLabel);
    sampleRateLabel.setText("Sample Rate: --", juce::dontSendNotification);
    sampleRateLabel.setFont(labelFont);
    sampleRateLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    sampleRateLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setText("Buffer: --", juce::dontSendNotification);
    bufferSizeLabel.setFont(labelFont);
    bufferSizeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    bufferSizeLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(portLabel);
    portLabel.setText("Port: --", juce::dontSendNotification);
    portLabel.setFont(labelFont);
    portLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    portLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(statusLabel);
    statusLabel.setText("Server: Starting...", juce::dontSendNotification);
    statusLabel.setFont(labelFont);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF888888));
    statusLabel.setJustificationType(juce::Justification::centred);
    
    addAndMakeVisible(devicesLabel);
    devicesLabel.setText("Devices: 0", juce::dontSendNotification);
    devicesLabel.setFont(labelFont);
    devicesLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF888888));
    devicesLabel.setJustificationType(juce::Justification::centred);
    
    startTimer(100);
}

FAUNAAudioProcessorEditor::~FAUNAAudioProcessorEditor()
{
    stopTimer();
}

void FAUNAAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0d0d0d));
    
    int centerX = getWidth() / 2;
    
    g.setColour(juce::Colour(0xFFD4AF00));
    g.setFont(titleFont);
    g.drawText("FAUNA", 0, 30, getWidth(), 70, juce::Justification::centred, false);
    
    int boxSize = 180;
    int boxX = centerX - boxSize / 2;
    int boxY = 120;
    
    g.setColour(juce::Colour(0xFF1a1a1a));
    g.fillRect(boxX, boxY, boxSize, boxSize);
    
    g.setColour(juce::Colour(0xFFD4AF00));
    g.drawRect(boxX, boxY, boxSize, boxSize, 3);
    
    if (!lastURL.isEmpty() && lastURL != "http://--.--.--.--:8080")
    {
        drawQRCode(g, lastURL, boxX + 10, boxY + 10, boxSize - 20);
    }
}

void FAUNAAudioProcessorEditor::drawQRCode(juce::Graphics& g, const juce::String& url, int x, int y, int size)
{
    if (url.isEmpty()) return;
    
    try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(url.toUTF8(), qrcodegen::QrCode::Ecc::LOW);
        int qrSize = qr.getSize();
        int moduleSize = size / qrSize;
        
        g.setColour(juce::Colours::white);
        g.fillRect(x, y, size, size);
        
        g.setColour(juce::Colours::black);
        for (int row = 0; row < qrSize; row++)
        {
            for (int col = 0; col < qrSize; col++)
            {
                if (qr.getModule(col, row))
                {
                    g.fillRect(x + col * moduleSize, y + row * moduleSize, moduleSize, moduleSize);
                }
            }
        }
        qrCodeValid = true;
    }
    catch (const std::exception&) {
        qrCodeValid = false;
    }
}

void FAUNAAudioProcessorEditor::resized()
{
    int w = getWidth();
    
    urlLabel.setBounds(0, 320, w, 30);
    
    int bottomY = 360;
    int labelWidth = w / 3;
    
    sampleRateLabel.setBounds(0, bottomY, labelWidth, 20);
    bufferSizeLabel.setBounds(labelWidth, bottomY, labelWidth, 20);
    portLabel.setBounds(labelWidth * 2, bottomY, labelWidth, 20);
    
    statusLabel.setBounds(0, 100, w, 20);
    devicesLabel.setBounds(0, 125, w, 20);
}

void FAUNAAudioProcessorEditor::timerCallback()
{
    static bool ipDetected = false;
    
    if (audioProcessor.isShuttingDown)
    {
        return;
    }
    
    if (!audioProcessor.isServerRunning())
    {
        statusLabel.setText("Server: Starting...", juce::dontSendNotification);
        return;
    }
    
    if (!ipDetected)
    {
        audioProcessor.httpServer.detectLocalIP();
        ipDetected = true;
    }
    
    statusLabel.setText("Server: Running", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00FF88));
    
    juce::String ip = audioProcessor.httpServer.getLocalIPAddress();
    juce::String urlText = "http://" + ip + ":8080";
    
    if (urlText != lastURL)
    {
        lastURL = urlText;
        qrCodeValid = false;
        repaint();
    }
    
    urlLabel.setText(urlText, juce::dontSendNotification);
    
    int sr = (int)audioProcessor.getSampleRate();
    int bs = audioProcessor.getBlockSize();
    int pt = audioProcessor.getServerPort();
    
    sampleRateLabel.setText(juce::String(sr) + " Hz", juce::dontSendNotification);
    bufferSizeLabel.setText("Buffer: " + juce::String(bs), juce::dontSendNotification);
    portLabel.setText("Port: " + juce::String(pt), juce::dontSendNotification);
    
    int connected = audioProcessor.getConnectedClients();
    devicesLabel.setText("Devices: " + juce::String(connected), juce::dontSendNotification);
    if (connected > 0)
        devicesLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00FF88));
    else
        devicesLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF888888));
}
