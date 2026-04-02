#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <map>

class WebServerConnection
{
public:
    juce::StreamingSocket* socket = nullptr;
    bool isWebSocket = false;
    bool muted = false;
    juce::String ipAddress;
};

class WebServer : public juce::Thread
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

private:
    void run() override;
    void handleClient(juce::StreamingSocket* clientSocket);
    juce::String getLocalIPAddress();
    juce::String generateQRCodeURL();

    int port = 8080;
    std::atomic<bool> running{ false };
    std::atomic<int> connectedClients{ 0 };
    juce::StreamingSocket* serverSocket = nullptr;

    std::map<int, WebServerConnection> connections;
    std::mutex connectionsMutex;
    int nextClientId = 0;

    juce::String webPageContent;

    void buildWebPage();
    juce::String handleHTTPRequest(const juce::String& request, juce::StreamingSocket* socket);
    void handleWebSocketFrame(juce::StreamingSocket* socket, const char* data, int length);
    void sendWebSocketMessage(juce::StreamingSocket* socket, const juce::String& message);
    juce::String generateWebSocketHandshake(const juce::String& key);

    std::atomic<bool> audioPlaying{ false };
    std::atomic<float> currentLevel{ 0.0f };

    JUCE_LEAK_DETECTOR(WebServer)
};
