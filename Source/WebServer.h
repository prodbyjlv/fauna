#pragma once

#include <JuceHeader.h>

class WebServer
{
public:
    WebServer();
    ~WebServer();

    bool start(int port = 8080);
    void stop();
    void setMuteState(int clientId, bool muted);
    void writeAudioData(const float* audioData, int numSamples);

    bool isRunning() const { return running.load(); }
    int getConnectedClients() const;
    int getPort() const { return port; }
    float getCurrentLevel() const;

private:
    juce::String getLocalIPAddress();

    int port = 8080;
    std::atomic<bool> running{ false };
    std::atomic<int> connectedClients{ 0 };
    std::atomic<float> currentLevel{ 0.0f };

    JUCE_LEAK_DETECTOR(WebServer)
};
