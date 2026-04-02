#pragma once

#include <JuceHeader.h>

class WebServer
{
public:
    WebServer();
    ~WebServer();

    bool start(int port = 8080);
    void stop();
    
    bool isRunning() const { return running.load(); }
    int getPort() const { return port; }
    int getConnectedClients() const { return connectedClients.load(); }
    juce::String getLocalIPAddress() const { return localIP; }
    float getCurrentLevel() const { return currentLevel.load(); }
    
    void setMuteState(int clientId, bool muted) {}
    void writeAudioData(const float* audioData, int numSamples) {}

private:
    void detectLocalIP();

    int port = 8080;
    std::atomic<bool> running{ false };
    std::atomic<int> connectedClients{ 0 };
    std::atomic<float> currentLevel{ 0.0f };
    juce::String localIP = "127.0.0.1";

    JUCE_LEAK_DETECTOR(WebServer)
};
