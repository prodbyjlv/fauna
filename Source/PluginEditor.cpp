#include "PluginEditor.h"

FAUNAAudioProcessorEditor::FAUNAAudioProcessorEditor (FAUNAAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (W, H);
    setResizable (false, false);
    startTimerHz (2);
}

FAUNAAudioProcessorEditor::~FAUNAAudioProcessorEditor()
{
    stopTimer();
}

void FAUNAAudioProcessorEditor::timerCallback()
{
    serverIP = audioProcessor.httpServer.getLocalIPAddress();
    serverPort = audioProcessor.httpServer.getPort();
    streamSampleRate = audioProcessor.httpServer.getSampleRate();
    connectedDevices = audioProcessor.httpServer.getConnectedClients();
    repaint();
}

void FAUNAAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (colBg);
    paintWatermark (g);
    paintMetaRow   (g);
    paintQRCode    (g);
    paintFooter    (g);
}

void FAUNAAudioProcessorEditor::resized() {}

void FAUNAAudioProcessorEditor::paintWatermark (juce::Graphics& g) const
{
    const float fontSize = 18.0f;
    const float tileW    = 86.0f;
    const float tileH    = 32.0f;
    const float angleDeg = -30.0f;

    juce::Font watermarkFont (juce::FontOptions()
                                  .withName  ("Arial")
                                  .withStyle ("Black")
                                  .withHeight (fontSize));

    g.setFont  (watermarkFont);
    g.setColour (colCoral.withAlpha (0.08f));

    const float diag = std::sqrt ((float)(W * W + H * H));
    const int   cols = (int)(diag / tileW) + 4;
    const int   rows = (int)(diag / tileH) + 4;

    g.saveState();
    g.addTransform (
        juce::AffineTransform()
            .translated (-diag * 0.5f, -diag * 0.5f)
            .rotated (juce::degreesToRadians (angleDeg))
            .translated (W * 0.5f, H * 0.5f)
    );

    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < cols; ++col)
            g.drawText ("FAUNA",
                        (int)(col * tileW),
                        (int)(row * tileH),
                        (int)tileW,
                        (int)tileH,
                        juce::Justification::left,
                        false);

    g.restoreState();
}

void FAUNAAudioProcessorEditor::paintMetaRow (juce::Graphics& g) const
{
    juce::Font labelFont (juce::FontOptions()
                              .withName ("Courier New")
                              .withHeight (9.0f));

    juce::Font valueFont (juce::FontOptions()
                              .withName ("Courier New")
                              .withHeight (10.0f));

    const int pad  = 18;
    const int topY = 18;

    auto drawMeta = [&](const juce::String& label,
                        const juce::String& value,
                        int x, int w,
                        juce::Justification just)
    {
        g.setFont   (labelFont);
        g.setColour (colCoral.withAlpha (0.8f));
        g.drawText  (label.toUpperCase(), x, topY, w, 13, just, false);

        g.setFont   (valueFont);
        g.setColour (colCream);
        g.drawText  (value, x, topY + 15, w, 14, just, false);
    };

    const juce::String url = "http://" + serverIP + ":" + juce::String (serverPort);
    const juce::String sr  = juce::String ((int) streamSampleRate) + " Hz";

    drawMeta ("URL",  url,                    pad,     200, juce::Justification::left);
    drawMeta ("Port", juce::String (serverPort), 200, 80,   juce::Justification::centred);
    drawMeta ("SR",   sr,                     280, 40, juce::Justification::right);

    const float divY = (float)(topY + 34);
    juce::ColourGradient grad (colCoral.withAlpha (0.45f), (float)pad,      divY,
                               colBg,                      (float)(W - pad), divY,
                               false);
    g.setGradientFill (grad);
    g.drawLine ((float)pad, divY, (float)(W - pad), divY, 1.0f);
}

void FAUNAAudioProcessorEditor::paintQRCode (juce::Graphics& g) const
{
    const int qrSize = 300;
    const int qrX    = (W - qrSize) / 2;
    const int qrY    = 60;

    juce::Rectangle<float> panel ((float)qrX, (float)qrY,
                                   (float)qrSize, (float)qrSize);

    g.setColour (colCream);
    g.fillRoundedRectangle (panel, 12.0f);

    paintQRGrid (g, panel.reduced (10.0f));
}

void FAUNAAudioProcessorEditor::paintQRGrid (juce::Graphics& g,
                                              juce::Rectangle<float> b) const
{
    juce::String url = "http://" + serverIP + ":" + juce::String (serverPort);
    
    try
    {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText (url.toUTF8(), qrcodegen::QrCode::Ecc::MEDIUM);
        int n = qr.getSize();
        float cw = b.getWidth()  / (float)n;
        float ch = b.getHeight() / (float)n;
        
        g.setColour (colBg);
        for (int row = 0; row < n; ++row)
        {
            for (int col = 0; col < n; ++col)
            {
                if (qr.getModule (col, row))
                {
                    g.fillRect (b.getX() + col * cw,
                                b.getY() + row * ch,
                                cw, ch);
                }
            }
        }
    }
    catch (...)
    {
    }
}

void FAUNAAudioProcessorEditor::paintFooter (juce::Graphics& g) const
{
    const float dotSize = 6.0f;
    const float dotX    = 18.0f;
    const float dotY    = (float)H - 18.0f;

    bool connected = connectedDevices > 0;
    g.setColour (connected ? colGreen : colCoral.withAlpha (0.4f));
    g.fillEllipse (dotX, dotY - dotSize * 0.5f, dotSize, dotSize);

    juce::Font font (juce::FontOptions()
                         .withName ("Courier New")
                         .withHeight (10.0f));
    g.setFont   (font);
    g.setColour (colCream.withAlpha (0.35f));

    juce::String txt = connected
        ? juce::String (connectedDevices)
              + (connectedDevices == 1 ? " device connected" : " devices connected")
        : "No devices connected";

    g.drawText (txt,
                (int)(dotX + dotSize + 6.0f),
                H - 24,
                220, 18,
                juce::Justification::centredLeft,
                false);
}
