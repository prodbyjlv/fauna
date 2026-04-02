#pragma once

#include <JuceHeader.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class HTTPClient
{
public:
    SOCKET socket = INVALID_SOCKET;
    bool muted = false;
    juce::String ipAddress;
};

class HTTPServer
{
public:
    HTTPServer();
    ~HTTPServer();

    bool start(int port = 8080);
    void stop();
    void processConnections();
    
    bool isRunning() const { return running.load(); }
    int getPort() const { return port; }
    int getConnectedClients() const { return clients.size(); }
    juce::String getLocalIPAddress() const { return localIP; }
    float getCurrentLevel() const { return currentLevel.load(); }
    
    void setMuteState(int clientId, bool muted) {}
    void writeAudioData(const float* audioData, int numSamples) {}
    
    void setLevel(float level) { currentLevel.store(level); }

private:
    void detectLocalIP();
    SOCKET createServerSocket(int port);
    void handleClient(SOCKET clientSocket);
    juce::String buildHTTPResponse(const juce::String& request);
    juce::String getHTMLPage();

    int port = 8080;
    std::atomic<bool> running{ false };
    std::atomic<float> currentLevel{ 0.0f };
    juce::String localIP = "127.0.0.1";

    SOCKET serverSocket = INVALID_SOCKET;
    juce::Array<HTTPClient> clients;

    JUCE_LEAK_DETECTOR(HTTPServer)
};
