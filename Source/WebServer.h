#pragma once

#include <JuceHeader.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <thread>
#endif

#include <atomic>

#ifdef _WIN32
    using SocketType = SOCKET;
    #define CLOSE_SOCKET closesocket
    #define SHUTDOWN_BOTH SD_BOTH
    #define GET_SOCKET_ERROR_CODE WSAGetLastError()
    #define DEBUG_OUTPUT(x) OutputDebugString(x)
    using ThreadReturnType = DWORD;
#else
    using SocketType = int;
    #define CLOSE_SOCKET close
    #define SHUTDOWN_BOTH SHUT_RDWR
    #define INVALID_SOCKET (-1)
    #define GET_SOCKET_ERROR_CODE errno
    #define DEBUG_OUTPUT(x) printf("%s", x)
    using ThreadReturnType = void*;
#endif

class AudioClient
{
public:
    SocketType socket = INVALID_SOCKET;
    bool muted = false;
    juce::String ipAddress;
    bool isWebSocket = false;
    int64_t lastPingTime = 0;
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
    int getConnectedClients() const { return connectedClientCount.load(); }

    void detectLocalIP();
    juce::String getLocalIPAddress() const { return localIP; }

    float getCurrentLevel() const { return currentLevel; }
    void setMuteState(int clientId, bool muted) {}
    void writeAudioData(const float* audioData, int numSamples);
    void setLevel(float level) { currentLevel = level; }
    void setSampleRate(double sr) { sampleRate = sr; }
    double getSampleRate() const { return sampleRate; }

private:
#ifdef _WIN32
    static DWORD WINAPI serverThreadFunc(LPVOID lpParam);
#else
    static void* serverThreadFunc(void* lpParam);
#endif
    ThreadReturnType serverThread();
    SocketType createServerSocket(int port);
    void handleClient(SocketType clientSocket);
    void handleWebSocketClient(AudioClient& client);
    juce::String buildHTTPResponse(const juce::String& request);
    juce::String getHTMLPage();
    juce::String generateWebSocketKey(const char* key);
    void sendWebSocketFrame(SocketType socket, const char* data, int length, int opcode);
    void broadcastAudio(const float* audioData, int numSamples);
    void sendSampleRateMessage(AudioClient& client);

    int port = 8080;
    volatile bool running = false;
    float currentLevel = 0.0f;
    juce::String localIP;
    double sampleRate = 44100.0;

    SocketType serverSocket = INVALID_SOCKET;
#ifdef _WIN32
    HANDLE serverThreadHandle = NULL;
#else
    std::thread serverThreadHandle;
#endif

    juce::Array<AudioClient> audioClients;
    juce::CriticalSection clientsLock;
    std::atomic<int> connectedClientCount = 0;

    JUCE_LEAK_DETECTOR(HTTPServer)
};