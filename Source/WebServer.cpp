#include "WebServer.h"
#include <cstring>

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Correct SHA-1 implementation
static void sha1(const char* data, int len, unsigned char* out)
{
    unsigned int h0 = 0x67452301;
    unsigned int h1 = 0xEFCDAB89;
    unsigned int h2 = 0x98BADCFE;
    unsigned int h3 = 0x10325476;
    unsigned int h4 = 0xC3D2E1F0;

    int totalLen = len + 1 + 8;
    if (totalLen % 64 != 0)
        totalLen = ((totalLen / 64) + 1) * 64;

    juce::HeapBlock<unsigned char> buffer(totalLen, true);
    std::memcpy(buffer.get(), data, len);
    buffer[len] = 0x80;

    unsigned long long bits = (unsigned long long)len * 8;
    for (int i = 7; i >= 0; i--)
    {
        buffer[totalLen - 8 + i] = (unsigned char)(bits & 0xFF);
        bits >>= 8;
    }

    for (int i = 0; i < totalLen; i += 64)
    {
        unsigned int w[80];
        for (int j = 0; j < 16; j++)
        {
            w[j] = ((unsigned int)buffer[i + j*4]     << 24) |
                   ((unsigned int)buffer[i + j*4 + 1] << 16) |
                   ((unsigned int)buffer[i + j*4 + 2] <<  8) |
                   ((unsigned int)buffer[i + j*4 + 3]);
        }
        for (int j = 16; j < 80; j++)
        {
            unsigned int val = w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16];
            w[j] = (val << 1) | (val >> 31);
        }

        unsigned int a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int j = 0; j < 20; j++) {
            unsigned int f = (b & c) | ((~b) & d);
            unsigned int temp = ((a<<5)|(a>>27)) + f + e + 0x5A827999 + w[j];
            e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
        }
        for (int j = 20; j < 40; j++) {
            unsigned int f = b ^ c ^ d;
            unsigned int temp = ((a<<5)|(a>>27)) + f + e + 0x6ED9EBA1 + w[j];
            e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
        }
        for (int j = 40; j < 60; j++) {
            unsigned int f = (b & c) | (b & d) | (c & d);
            unsigned int temp = ((a<<5)|(a>>27)) + f + e + 0x8F1BBCDC + w[j];
            e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
        }
        for (int j = 60; j < 80; j++) {
            unsigned int f = b ^ c ^ d;
            unsigned int temp = ((a<<5)|(a>>27)) + f + e + 0xCA62C1D6 + w[j];
            e=d; d=c; c=(b<<30)|(b>>2); b=a; a=temp;
        }

        h0+=a; h1+=b; h2+=c; h3+=d; h4+=e;
    }

    out[0]=(h0>>24)&0xFF; out[1]=(h0>>16)&0xFF; out[2]=(h0>>8)&0xFF;  out[3]=h0&0xFF;
    out[4]=(h1>>24)&0xFF; out[5]=(h1>>16)&0xFF; out[6]=(h1>>8)&0xFF;  out[7]=h1&0xFF;
    out[8]=(h2>>24)&0xFF; out[9]=(h2>>16)&0xFF; out[10]=(h2>>8)&0xFF; out[11]=h2&0xFF;
    out[12]=(h3>>24)&0xFF; out[13]=(h3>>16)&0xFF; out[14]=(h3>>8)&0xFF; out[15]=h3&0xFF;
    out[16]=(h4>>24)&0xFF; out[17]=(h4>>16)&0xFF; out[18]=(h4>>8)&0xFF; out[19]=h4&0xFF;
}

// Correct base64 encoding
static juce::String base64Encode(const unsigned char* data, int len)
{
    juce::String result;
    int i = 0;
    while (i < len)
    {
        int remaining = len - i;
        int a = data[i++];
        int b = (remaining > 1) ? data[i++] : 0;
        int c = (remaining > 2) ? data[i++] : 0;

        result += base64_chars[a >> 2];
        result += base64_chars[((a & 3) << 4) | (b >> 4)];
        result += (remaining > 1) ? base64_chars[((b & 15) << 2) | (c >> 6)] : '=';
        result += (remaining > 2) ? base64_chars[c & 63] : '=';
    }
    return result;
}

//==============================================================================
HTTPServer::HTTPServer()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    localIP = "127.0.0.1";
}

HTTPServer::~HTTPServer()
{
    stop();
    WSACleanup();
}

void HTTPServer::detectLocalIP()
{
    juce::Array<juce::IPAddress> addresses;
    juce::IPAddress::findAllAddresses(addresses);
    juce::String resultIP = "127.0.0.1";
    for (int i = 0; i < addresses.size(); i++)
    {
        juce::String ip = addresses[i].toString();
        if (ip.startsWith("192.168.") || ip.startsWith("10.") || ip.startsWith("172."))
        {
            resultIP = ip;
            break;
        }
    }
    localIP = resultIP;
}

bool HTTPServer::start(int targetPort)
{
    port = targetPort;
    serverSocket = createServerSocket(port);
    if (serverSocket == INVALID_SOCKET)
    {
        OutputDebugString("FAUNA: Failed to create socket\n");
        return false;
    }
    running = true;
    detectLocalIP();
    OutputDebugString("FAUNA: Starting server thread\n");
    serverThreadHandle = CreateThread(NULL, 0, serverThreadFunc, this, 0, NULL);
    if (serverThreadHandle == NULL)
    {
        OutputDebugString("FAUNA: Failed to create thread\n");
        running = false;
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
        return false;
    }
    OutputDebugString(("FAUNA: Server started on " + localIP + ":" + juce::String(port) + "\n").toUTF8());
    return true;
}

void HTTPServer::stop()
{
    running = false;
    if (serverSocket != INVALID_SOCKET) {
        shutdown(serverSocket, SD_BOTH);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    if (serverThreadHandle != NULL)
    {
        DWORD waitResult = WaitForSingleObject(serverThreadHandle, 2000);
        if (waitResult == WAIT_TIMEOUT)
            TerminateThread(serverThreadHandle, 0);
        CloseHandle(serverThreadHandle);
        serverThreadHandle = NULL;
    }
    {
        juce::ScopedLock lock(clientsLock);
        for (auto& client : audioClients)
        {
            if (client.socket != INVALID_SOCKET) {
                shutdown(client.socket, SD_BOTH);
                closesocket(client.socket);
                client.socket = INVALID_SOCKET;
            }
        }
        audioClients.clear();
    }
    OutputDebugString("FAUNA: Server fully stopped\n");
}

DWORD WINAPI HTTPServer::serverThreadFunc(LPVOID lpParam)
{
    HTTPServer* server = (HTTPServer*)lpParam;
    return server->serverThread();
}

DWORD HTTPServer::serverThread()
{
    OutputDebugString("FAUNA: Server thread started\n");

    // Set accept timeout so we can check running flag regularly
    DWORD timeout = 100; // ms
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (running && serverSocket != INVALID_SOCKET)
    {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);

        if (clientSocket != INVALID_SOCKET)
        {
            OutputDebugString("FAUNA: Client connected\n");
            handleClient(clientSocket);
        }

        // Check existing WebSocket clients for incoming frames (mute/control messages)
        {
            juce::ScopedLock lock(clientsLock);
            for (int i = audioClients.size() - 1; i >= 0; i--)
            {
                AudioClient& client = audioClients.getReference(i);
                if (client.socket == INVALID_SOCKET)
                {
                    audioClients.remove(i);
                    continue;
                }

                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(client.socket, &readSet);
                TIMEVAL tv = { 0, 0 }; // non-blocking check
                int result = select(0, &readSet, nullptr, nullptr, &tv);
                if (result > 0)
                    handleWebSocketClient(client);
                else if (result < 0)
                    audioClients.remove(i);
            }
        }
    }

    OutputDebugString("FAUNA: Server thread exiting\n");
    return 0;
}

SOCKET HTTPServer::createServerSocket(int port)
{
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
        return INVALID_SOCKET;

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons((u_short)port);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    if (listen(listenSocket, 5) == SOCKET_ERROR) {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }
    return listenSocket;
}

void HTTPServer::handleClient(SOCKET clientSocket)
{
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0)
    {
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    juce::String request(buffer);

    if (request.contains("Upgrade") && request.contains("websocket"))
    {
        OutputDebugString("FAUNA: WebSocket upgrade request detected\n");

        // Check client limit
        {
            juce::ScopedLock lock(clientsLock);
            if (audioClients.size() >= 2)
            {
                OutputDebugString("FAUNA: Max clients reached, rejecting\n");
                closesocket(clientSocket);
                return;
            }
        }

        // Parse Sec-WebSocket-Key
        const char* keyPtr = strstr(buffer, "Sec-WebSocket-Key:");
        juce::String wsKey = "dGhlIHNhbXBsZSBub25jZQ==";
        if (keyPtr != nullptr)
        {
            keyPtr += 18;
            while (*keyPtr == ' ') keyPtr++;
            const char* endPtr = strstr(keyPtr, "\r\n");
            if (endPtr != nullptr && endPtr > keyPtr)
                wsKey = juce::String(keyPtr, (int)(endPtr - keyPtr));
        }

        OutputDebugString(("FAUNA: WebSocket key: " + wsKey + "\n").toUTF8());

        std::string acceptKeyInput = wsKey.toStdString() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        unsigned char hash[20];
        sha1(acceptKeyInput.c_str(), (int)acceptKeyInput.size(), hash);
        juce::String acceptKey = base64Encode(hash, 20);

        OutputDebugString(("FAUNA: Accept key: " + acceptKey + "\n").toUTF8());

        juce::String response = "HTTP/1.1 101 Switching Protocols\r\n";
        response += "Upgrade: websocket\r\n";
        response += "Connection: Upgrade\r\n";
        response += "Sec-WebSocket-Accept: " + acceptKey + "\r\n";
        response += "\r\n";

        int sent = send(clientSocket, response.toUTF8(), response.length(), 0);
        OutputDebugString(("FAUNA: 101 sent, bytes: " + juce::String(sent) + "\n").toUTF8());

        // Set socket to non-blocking for audio streaming
        u_long nonBlocking = 1;
        ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

        // Set send buffer large enough for audio
        int sendBuf = 256 * 1024;
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBuf, sizeof(sendBuf));

        AudioClient client;
        client.socket = clientSocket;
        client.isWebSocket = true;
        client.muted = false;
        client.lastPingTime = 0;

        sockaddr_in addr;
        int addrLen = sizeof(addr);
        if (getpeername(clientSocket, (sockaddr*)&addr, &addrLen) == 0)
        {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
            client.ipAddress = ipStr;
        }

        juce::ScopedLock lock(clientsLock);
        audioClients.add(client);
        OutputDebugString(("FAUNA: Client added. Total: " + juce::String(audioClients.size()) + "\n").toUTF8());
    }
    else
    {
        // Regular HTTP request
        juce::String response = buildHTTPResponse(request);
        send(clientSocket, response.toUTF8(), response.length(), 0);
        closesocket(clientSocket);
    }
}

void HTTPServer::handleWebSocketClient(AudioClient& client)
{
    char buffer[8192];
    int bytesReceived = recv(client.socket, buffer, sizeof(buffer), 0);

    if (bytesReceived == 0 || (bytesReceived < 0 && WSAGetLastError() != WSAEWOULDBLOCK))
    {
        OutputDebugString("FAUNA: Client disconnected\n");
        closesocket(client.socket);
        client.socket = INVALID_SOCKET;
        return;
    }

    if (bytesReceived < 2) return;

    unsigned char opcode = buffer[0] & 0x0F;
    bool masked = (buffer[1] & 0x80) != 0;
    unsigned int payloadLength = buffer[1] & 0x7F;

    int headerLen = 2;
    if (payloadLength == 126) headerLen += 2;
    if (payloadLength == 127) headerLen += 8;
    if (masked) headerLen += 4;

    if (opcode == 0x01) // Text frame (mute control)
    {
        if (bytesReceived >= headerLen)
        {
            char* data = buffer + headerLen;
            int dataLen = bytesReceived - headerLen;
            if (masked && dataLen > 0)
            {
                char* maskKey = buffer + headerLen - 4;
                for (int i = 0; i < dataLen; i++)
                    data[i] ^= maskKey[i % 4];
            }
            juce::String message(data, dataLen);
            OutputDebugString(("FAUNA: Control message: " + message + "\n").toUTF8());
            if (message.contains("mute"))
                client.muted = message.contains("true");
        }
    }
    else if (opcode == 0x08) // Close frame
    {
        OutputDebugString("FAUNA: Close frame, disconnecting client\n");
        sendWebSocketFrame(client.socket, nullptr, 0, 0x08);
        closesocket(client.socket);
        client.socket = INVALID_SOCKET;
    }
    else if (opcode == 0x09) // Ping
    {
        char pongPayload[125];
        int pongLen = 0;
        if (payloadLength > 0 && payloadLength <= 125 && bytesReceived > headerLen)
        {
            pongLen = (int)payloadLength;
            memcpy(pongPayload, buffer + headerLen, pongLen);
            if (masked)
            {
                char* maskKey = buffer + headerLen - 4;
                for (int i = 0; i < pongLen; i++)
                    pongPayload[i] ^= maskKey[i % 4];
            }
        }
        sendWebSocketFrame(client.socket, pongLen > 0 ? pongPayload : nullptr, pongLen, 0x0A);
    }
}

juce::String HTTPServer::generateWebSocketKey(const char* key)
{
    std::string combined = std::string(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char hash[20];
    sha1(combined.c_str(), (int)combined.size(), hash);
    return base64Encode(hash, 20);
}

void HTTPServer::sendWebSocketFrame(SOCKET socket, const char* data, int length, int opcode)
{
    if (socket == INVALID_SOCKET) return;

    // Build header + payload in one contiguous buffer to avoid two TCP packets
    int headerLen = 2;
    if (length >= 126 && length < 65536) headerLen = 4;
    else if (length >= 65536) headerLen = 10;

    int totalLen = headerLen + length;
    juce::HeapBlock<char> frame(totalLen);

    frame[0] = (char)(0x80 | (opcode & 0x0F)); // FIN + opcode
    // Server MUST NOT mask frames sent to client (RFC 6455 section 5.1)

    if (length < 126)
    {
        frame[1] = (char)(length & 0x7F);
    }
    else if (length < 65536)
    {
        frame[1] = 126;
        frame[2] = (char)((length >> 8) & 0xFF);
        frame[3] = (char)(length & 0xFF);
    }
    else
    {
        frame[1] = 127;
        // 8-byte big-endian length
        for (int i = 0; i < 6; i++) frame[2 + i] = 0;
        frame[8] = (char)((length >> 8) & 0xFF);
        frame[9] = (char)(length & 0xFF);
    }

    if (length > 0 && data != nullptr)
        memcpy(frame.get() + headerLen, data, length);

    send(socket, frame.get(), totalLen, 0);
}

//==============================================================================
// THE KEY FIX: broadcastAudio now actually sends audio to all connected clients
//==============================================================================
void HTTPServer::broadcastAudio(const float* audioData, int numSamples)
{
    // audioData is interleaved stereo: [L0, R0, L1, R1, ...]
    // numSamples is per-channel sample count
    // Total floats = numSamples * 2

    int numFloats = numSamples * 2;
    int byteCount = numFloats * sizeof(float);

    juce::ScopedLock lock(clientsLock);

    if (audioClients.size() == 0) return;

    for (int i = audioClients.size() - 1; i >= 0; i--)
    {
        AudioClient& client = audioClients.getReference(i);

        if (client.socket == INVALID_SOCKET)
        {
            audioClients.remove(i);
            continue;
        }

        if (!client.isWebSocket) continue;

        // If muted, send silence instead of real audio
        if (client.muted)
        {
            juce::HeapBlock<float> silence(numFloats, true); // zero-initialised
            sendWebSocketFrame(client.socket, (const char*)silence.get(), byteCount, 0x02); // binary frame
        }
        else
        {
            sendWebSocketFrame(client.socket, (const char*)audioData, byteCount, 0x02); // binary frame
        }
    }
}

void HTTPServer::writeAudioData(const float* audioData, int numSamples)
{
    broadcastAudio(audioData, numSamples);
}

//==============================================================================
juce::String HTTPServer::getHTMLPage()
{
    // Serve the index.html content inline
    // This is the full mobile page that connects via WebSocket and plays audio
    juce::String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html += "<title>FAUNA Audio Stream</title>";
    html += "<style>";
    html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
    html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
    html += "background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);";
    html += "min-height: 100vh; display: flex; flex-direction: column;";
    html += "align-items: center; justify-content: center; color: white; padding: 20px; }";
    html += ".container { background: rgba(255,255,255,0.1); border-radius: 20px; padding: 40px;";
    html += "text-align: center; backdrop-filter: blur(10px); max-width: 400px; width: 100%; }";
    html += "h1 { font-size: 2em; margin-bottom: 10px; color: #00d4ff; }";
    html += ".status { font-size: 0.9em; color: #aaa; margin-bottom: 30px; }";
    html += ".level-meter { width: 100%; height: 30px; background: rgba(0,0,0,0.3);";
    html += "border-radius: 15px; overflow: hidden; margin-bottom: 30px; }";
    html += ".level-bar { height: 100%; width: 0%; background: linear-gradient(90deg, #00ff88, #00d4ff, #ffaa00, #ff4444);";
    html += "transition: width 0.1s; border-radius: 15px; }";
    html += ".mute-btn { padding: 20px 60px; font-size: 1.5em; border: none; border-radius: 15px;";
    html += "cursor: pointer; transition: all 0.3s; font-weight: bold; }";
    html += ".mute-btn.muted { background: #ff4444; color: white; }";
    html += ".mute-btn.unmuted { background: #00ff88; color: #1a1a2e; }";
    html += ".mute-btn:hover { transform: scale(1.05); }";
    html += ".connection-status { margin-top: 20px; font-size: 0.8em; color: #888; }";
    html += ".connected { color: #00ff88; }";
    html += ".info-box { background: rgba(0,0,0,0.2); padding: 15px; border-radius: 10px; margin-bottom: 20px; }";
    html += ".info-box p { margin: 5px 0; }";
    html += ".start-btn { padding: 15px 40px; font-size: 1.2em; background: #00d4ff; color: #1a1a2e;";
    html += "border: none; border-radius: 10px; cursor: pointer; margin-bottom: 20px; transition: all 0.3s; }";
    html += ".start-btn:hover { transform: scale(1.05); }";
    html += ".start-btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>FAUNA</h1>";
    html += "<p class=\"status\">Audio Streaming Control</p>";
    html += "<div class=\"info-box\">";
    html += "<p><strong>Server:</strong> <span id=\"serverStatus\">Checking...</span></p>";
    html += "<p><strong>Devices:</strong> <span id=\"deviceCount\">0</span></p>";
    html += "<p><strong>Audio:</strong> <span id=\"audioStatus\">Click Start to begin</span></p>";
    html += "</div>";
    html += "<button class=\"start-btn\" id=\"startBtn\" onclick=\"startAudio()\">START AUDIO</button>";
    html += "<div class=\"level-meter\"><div class=\"level-bar\" id=\"levelBar\"></div></div>";
    html += "<button class=\"mute-btn unmuted\" id=\"muteBtn\" onclick=\"toggleMute()\">UNMUTED</button>";
    html += "<p class=\"connection-status\" id=\"status\">Ready</p>";
    html += "</div>";
    html += "<script>";
    // Audio playback using Web Audio API with a queue-based approach
    html += "var isMuted=false,ws=null,audioCtx=null,started=false;";
    html += "var audioQueue=[],isPlaying=false,nextPlayTime=0;";
    html += "var SAMPLE_RATE=44100,CHANNELS=2,CHUNK_SIZE=4096;";
    // startAudio: called on button press (required for AudioContext on mobile)
    html += "function startAudio(){";
    html += "if(started)return; started=true;";
    html += "var btn=document.getElementById('startBtn');";
    html += "btn.textContent='Connecting...'; btn.disabled=true;";
    html += "document.getElementById('audioStatus').textContent='Connecting...';";
    // Create AudioContext - mobile browsers need user gesture first
    html += "audioCtx=new(window.AudioContext||window.webkitAudioContext)({sampleRate:SAMPLE_RATE});";
    html += "SAMPLE_RATE=audioCtx.sampleRate;";
    // Connect WebSocket
    html += "ws=new WebSocket('ws://'+location.host);";
    html += "ws.binaryType='arraybuffer';";
    html += "ws.onopen=function(){";
    html += "document.getElementById('audioStatus').textContent='Playing!';";
    html += "document.getElementById('audioStatus').className='connected';";
    html += "btn.textContent='Streaming...';";
    html += "nextPlayTime=audioCtx.currentTime+0.1;"; // small initial buffer
    html += "};";
    html += "ws.onclose=function(e){";
    html += "document.getElementById('audioStatus').textContent='Disconnected ('+e.code+')';";
    html += "document.getElementById('audioStatus').style.color='#ff4444';";
    html += "btn.disabled=false; btn.textContent='RECONNECT'; started=false;";
    html += "};";
    html += "ws.onerror=function(){document.getElementById('audioStatus').textContent='Error';};";
    // onmessage: receive Float32 interleaved stereo, decode and schedule for playback
    html += "ws.onmessage=function(e){";
    html += "if(!(e.data instanceof ArrayBuffer))return;";
    html += "var floats=new Float32Array(e.data);";
    html += "var numFrames=floats.length/2;";
    html += "if(numFrames<1)return;";
    // Create an AudioBuffer and fill L/R channels
    html += "var buf=audioCtx.createBuffer(2,numFrames,SAMPLE_RATE);";
    html += "var L=buf.getChannelData(0),R=buf.getChannelData(1);";
    html += "for(var i=0;i<numFrames;i++){L[i]=floats[i*2];R[i]=floats[i*2+1];}";
    // Schedule playback - keep a small rolling buffer ahead of current time
    html += "if(nextPlayTime<audioCtx.currentTime+0.05)";
    html += "nextPlayTime=audioCtx.currentTime+0.05;"; // re-sync if we fall behind
    html += "var src=audioCtx.createBufferSource();";
    html += "src.buffer=buf;";
    html += "src.connect(audioCtx.destination);";
    html += "src.start(nextPlayTime);";
    html += "nextPlayTime+=buf.duration;";
    // Level meter
    html += "var maxLevel=0;";
    html += "for(var i=0;i<Math.min(200,L.length);i++){var a=Math.abs(L[i]);if(a>maxLevel)maxLevel=a;}";
    html += "document.getElementById('levelBar').style.width=(maxLevel*100)+'%';";
    html += "};";
    html += "}"; // end startAudio
    html += "function toggleMute(){";
    html += "isMuted=!isMuted;";
    html += "var btn=document.getElementById('muteBtn');";
    html += "if(isMuted){btn.textContent='MUTED';btn.className='mute-btn muted';}";
    html += "else{btn.textContent='UNMUTED';btn.className='mute-btn unmuted';}";
    html += "document.getElementById('status').textContent=isMuted?'Muted':'Unmuted';";
    html += "if(ws&&ws.readyState===WebSocket.OPEN)ws.send(isMuted?'mute:true':'mute:false');";
    html += "}";
    html += "function updateStatus(){";
    html += "fetch('/status').then(function(r){return r.json();})";
    html += ".then(function(d){";
    html += "document.getElementById('serverStatus').textContent='Running';";
    html += "document.getElementById('serverStatus').className='connected';";
    html += "document.getElementById('deviceCount').textContent=d.clients;";
    html += "}).catch(function(){";
    html += "document.getElementById('serverStatus').textContent='Not Running';";
    html += "document.getElementById('serverStatus').style.color='#ff4444';";
    html += "});";
    html += "}";
    html += "updateStatus(); setInterval(updateStatus,2000);";
    html += "</script></body></html>";
    return html;
}

juce::String HTTPServer::buildHTTPResponse(const juce::String& request)
{
    if (request.startsWith("GET / ") || request.startsWith("GET /index.html") ||
        request.startsWith("GET / HTTP") || request == "GET /")
    {
        juce::String htmlContent = getHTMLPage();
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html; charset=utf-8\r\n";
        response += "Content-Length: " + juce::String(htmlContent.getNumBytesAsUTF8()) + "\r\n";
        response += "Connection: close\r\n";
        response += "Access-Control-Allow-Origin: *\r\n\r\n";
        response += htmlContent;
        return response;
    }
    else if (request.startsWith("GET /status"))
    {
        juce::String status = "{\"clients\":" + juce::String(getConnectedClients()) +
                              ",\"port\":" + juce::String(port) +
                              ",\"ip\":\"" + localIP + "\"}";
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + juce::String(status.length()) + "\r\n";
        response += "Access-Control-Allow-Origin: *\r\n\r\n";
        response += status;
        return response;
    }
    else
    {
        juce::String body = "<html><body><h1>FAUNA</h1><p><a href='/'>Home</a></p></body></html>";
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + juce::String(body.length()) + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += body;
        return response;
    }
}
