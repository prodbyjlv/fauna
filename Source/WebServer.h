#pragma once

#include <JuceHeader.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct AudioClient
{
    SOCKET socket = INVALID_SOCKET;
    bool muted = false;
    juce::String ipAddress;
    bool isWebSocket = false;
    __int64 lastPingTime = 0;
    bool sampleRateSent = false;
};

class HTTPServer
{
public:
    HTTPServer();
    ~HTTPServer();

    bool start(int port = 8080);
    void stop();
    
    bool isRunning() const { return running; }
    int getPort() const { return port; }
    int getConnectedClients() const { return audioClients.size(); }
    void detectLocalIP();
    juce::String getLocalIPAddress() const { return localIP; }
    float getCurrentLevel() const { return currentLevel; }
    
    void setMuteState(int clientId, bool muted) {}
    void writeAudioData(const float* audioData, int numSamples);
    void setLevel(float level) { currentLevel = level; }
    void setSampleRate(double sr) { sampleRate = sr; }

private:
    static DWORD WINAPI serverThreadFunc(LPVOID lpParam);
    DWORD serverThread();
    SOCKET createServerSocket(int port);
    void handleClient(SOCKET clientSocket);
    void handleWebSocketClient(AudioClient& client);
    juce::String buildHTTPResponse(const juce::String& request);
    juce::String getHTMLPage();
    juce::String generateWebSocketKey(const char* key);
    void sendWebSocketFrame(SOCKET socket, const char* data, int length, int opcode);
    void broadcastAudio(const float* audioData, int numSamples);
    void sendSampleRateMessage(AudioClient& client);

    int port = 8080;
    volatile bool running = false;
    float currentLevel = 0.0f;
    juce::String localIP;
    double sampleRate = 44100.0;
    int audioFrameCount = 0;

    SOCKET serverSocket = INVALID_SOCKET;
    HANDLE serverThreadHandle = NULL;
    juce::Array<AudioClient> audioClients;
    juce::CriticalSection clientsLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HTTPServer)
};
