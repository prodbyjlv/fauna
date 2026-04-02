#include "WebServer.h"
#include <cstring>

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void sha1(const char* data, int len, unsigned char* out) {
    unsigned int h0 = 0x67452301;
    unsigned int h1 = 0xEFCDAB89;
    unsigned int h2 = 0x98BADCFE;
    unsigned int h3 = 0x10325476;
    unsigned int h4 = 0xC3D2E1F0;
    
    int padding = (56 - (len % 64)) % 64 + 64;
    int totalLen = len + padding + 8;
    
    juce::HeapBlock<char> buffer(totalLen);
    std::memcpy(buffer.get(), data, len);
    buffer[len] = 0x80;
    std::memset(buffer.get() + len + 1, 0, padding - 1);
    
    unsigned long long bits = len * 8;
    for (int i = 0; i < 8; i++) {
        buffer[len + padding - 1 - i] = (bits >> (i * 8)) & 0xFF;
    }
    
    for (int i = 0; i < totalLen; i += 64) {
        unsigned int w[80];
        for (int j = 0; j < 16; j++) {
            w[j] = ((unsigned char)buffer[i + j * 4] << 24) |
                   ((unsigned char)buffer[i + j * 4 + 1] << 16) |
                   ((unsigned char)buffer[i + j * 4 + 2] << 8) |
                   ((unsigned char)buffer[i + j * 4 + 3]);
        }
        for (int j = 16; j < 80; j++) {
            w[j] = ((w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16]) << 1) | ((w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16]) >> 31);
        }
        
        unsigned int a = h0, b = h1, c = h2, d = h3, e = h4;
        
        for (int j = 0; j < 20; j++) {
            unsigned int f = (b & c) | ((~b) & d);
            unsigned int k = 0x5A827999;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = ((b << 30) | (b >> 2)); b = a; a = temp;
        }
        for (int j = 20; j < 40; j++) {
            unsigned int f = b ^ c ^ d;
            unsigned int k = 0x6ED9EBA1;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = ((b << 30) | (b >> 2)); b = a; a = temp;
        }
        for (int j = 40; j < 60; j++) {
            unsigned int f = (b & c) | (b & d) | (c & d);
            unsigned int k = 0x8F1BBCDC;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = ((b << 30) | (b >> 2)); b = a; a = temp;
        }
        for (int j = 60; j < 80; j++) {
            unsigned int f = b ^ c ^ d;
            unsigned int k = 0xCA62C1D6;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = ((b << 30) | (b >> 2)); b = a; a = temp;
        }
        
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    
    out[0] = (h0 >> 24) & 0xFF; out[1] = (h0 >> 16) & 0xFF;
    out[2] = (h0 >> 8) & 0xFF; out[3] = h0 & 0xFF;
    out[4] = (h1 >> 24) & 0xFF; out[5] = (h1 >> 16) & 0xFF;
    out[6] = (h1 >> 8) & 0xFF; out[7] = h1 & 0xFF;
    out[8] = (h2 >> 24) & 0xFF; out[9] = (h2 >> 16) & 0xFF;
    out[10] = (h2 >> 8) & 0xFF; out[11] = h2 & 0xFF;
    out[12] = (h3 >> 24) & 0xFF; out[13] = (h3 >> 16) & 0xFF;
    out[14] = (h3 >> 8) & 0xFF; out[15] = h3 & 0xFF;
    out[16] = (h4 >> 24) & 0xFF; out[17] = (h4 >> 16) & 0xFF;
    out[18] = (h4 >> 8) & 0xFF; out[19] = h4 & 0xFF;
}

static juce::String base64Encode(const unsigned char* data, int len) {
    juce::String result;
    int i = 0;
    while (i < len) {
        int a = data[i++];
        int b = i < len ? data[i++] : 0;
        int c = i < len ? data[i++] : 0;
        
        int enc1 = a >> 2;
        int enc2 = ((a & 3) << 4) | (b >> 4);
        int enc3 = ((b & 15) << 2) | (c >> 6);
        int enc4 = c & 63;
        
        result += base64_chars[enc1];
        result += base64_chars[enc2];
        result += (i - 1 < len) ? base64_chars[enc3] : '=';
        result += (i < len) ? base64_chars[enc4] : '=';
    }
    return result;
}

HTTPServer::HTTPServer()
{
    detectLocalIP();
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
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
    
    for (auto& addr : addresses)
    {
        juce::String ip = addr.toString();
        if (ip.startsWith("192.168.") || ip.startsWith("10.") || ip.startsWith("172."))
        {
            localIP = ip;
            break;
        }
    }
    
    if (localIP == "127.0.0.1" && addresses.size() > 0)
        localIP = addresses[0].toString();
}

bool HTTPServer::start(int targetPort)
{
    port = targetPort;
    serverSocket = createServerSocket(port);
    if (serverSocket == INVALID_SOCKET)
        return false;
    running = true;
    return true;
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

void HTTPServer::stop()
{
    running = false;
    
    if (serverSocket != INVALID_SOCKET) {
        shutdown(serverSocket, SD_BOTH);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    {
        juce::ScopedLock lock(clientsLock);
        for (auto& client : audioClients) {
            if (client.socket != INVALID_SOCKET) {
                shutdown(client.socket, SD_BOTH);
                closesocket(client.socket);
            }
        }
        audioClients.clear();
    }
}

void HTTPServer::processConnections()
{
    if (!running || serverSocket == INVALID_SOCKET)
        return;
    
    fd_set readSet;
    FD_ZERO(&readSet);
    if (serverSocket != INVALID_SOCKET)
        FD_SET(serverSocket, &readSet);
    
    TIMEVAL timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    
    int result = select(0, &readSet, nullptr, nullptr, &timeout);
    
    if (result > 0 && serverSocket != INVALID_SOCKET && FD_ISSET(serverSocket, &readSet))
    {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket != INVALID_SOCKET)
            handleClient(clientSocket);
    }
    
    juce::Array<SOCKET> toRemove;
    
    {
        juce::ScopedLock lock(clientsLock);
        for (auto& client : audioClients)
        {
            if (client.socket != INVALID_SOCKET && client.isWebSocket)
            {
                fd_set clientSet;
                FD_ZERO(&clientSet);
                FD_SET(client.socket, &clientSet);
                timeout.tv_usec = 0;
                
                result = select(0, &clientSet, nullptr, nullptr, &timeout);
                if (result > 0)
                {
                    char buf[4096];
                    int recvResult = recv(client.socket, buf, sizeof(buf), 0);
                    if (recvResult <= 0)
                        toRemove.add(client.socket);
                    else
                        handleWebSocketClient(client);
                }
            }
        }
        
        for (auto sock : toRemove)
        {
            for (int i = audioClients.size() - 1; i >= 0; --i)
            {
                if (audioClients[i].socket == sock)
                {
                    if (audioClients[i].socket != INVALID_SOCKET)
                        closesocket(audioClients[i].socket);
                    audioClients.remove(i);
                }
            }
        }
    }
}

void HTTPServer::handleClient(SOCKET clientSocket)
{
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        juce::String request(buffer);
        
        if (request.contains("Upgrade") && request.contains("websocket"))
        {
            juce::String response = "HTTP/1.1 101 Switching Protocols\r\n";
            response += "Upgrade: websocket\r\n";
            response += "Connection: Upgrade\r\n";
            response += "Sec-WebSocket-Accept: " + generateWebSocketKey("dGhlIHNhbXBsZSBub25jZQ==") + "\r\n";
            response += "\r\n";
            
            send(clientSocket, response.toUTF8(), response.length(), 0);
            
            AudioClient client;
            client.socket = clientSocket;
            client.isWebSocket = true;
            client.muted = false;
            
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
        }
        else
        {
            juce::String response = buildHTTPResponse(request);
            send(clientSocket, response.toUTF8(), response.length(), 0);
            closesocket(clientSocket);
        }
    }
    else
    {
        closesocket(clientSocket);
    }
}

void HTTPServer::handleWebSocketClient(AudioClient& client)
{
    char buffer[4096];
    int bytesReceived = recv(client.socket, buffer, sizeof(buffer), 0);
    
    if (bytesReceived > 2)
    {
        unsigned char opcode = buffer[0] & 0x0F;
        
        if (opcode == 0x01)
        {
            int headerLen = 2;
            int length = buffer[1] & 0x7F;
            if (length == 126) headerLen = 4;
            if (length == 127) headerLen = 10;
            
            if (bytesReceived > headerLen)
            {
                juce::String message(buffer + headerLen, bytesReceived - headerLen);
                if (message.contains("mute"))
                    client.muted = message.contains("true");
            }
        }
        else if (opcode == 0x08)
        {
            client.socket = INVALID_SOCKET;
        }
    }
}

juce::String HTTPServer::generateWebSocketKey(const char* key)
{
    juce::String acceptKey = juce::String(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    unsigned char hash[20];
    sha1(acceptKey.toUTF8(), acceptKey.length(), hash);
    
    return base64Encode(hash, 20);
}

void HTTPServer::sendWebSocketFrame(SOCKET socket, const char* data, int length, int opcode)
{
    if (socket == INVALID_SOCKET || length <= 0)
        return;
    
    juce::MemoryOutputStream stream;
    stream.writeByte(0x80 | (opcode & 0x0F));
    
    if (length < 126)
        stream.writeByte(length);
    else if (length < 65536)
    {
        stream.writeByte(126);
        stream.writeByte((length >> 8) & 0xFF);
        stream.writeByte(length & 0xFF);
    }
    
    stream.write(data, length);
    send(socket, (const char*)stream.getData(), (int)stream.getDataSize(), 0);
}

void HTTPServer::broadcastAudio(const float* audioData, int numSamples)
{
    juce::ScopedLock lock(clientsLock);
    
    for (auto& client : audioClients)
    {
        if (client.socket != INVALID_SOCKET && !client.muted)
        {
            sendWebSocketFrame(client.socket, (const char*)audioData, numSamples * 2 * sizeof(float), 0x02);
        }
    }
}

void HTTPServer::writeAudioData(const float* audioData, int numSamples)
{
    broadcastAudio(audioData, numSamples);
}

juce::String HTTPServer::getHTMLPage()
{
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FAUNA Audio Stream</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%); min-height: 100vh; display: flex; flex-direction: column; align-items: center; justify-content: center; color: white; padding: 20px; }
        .container { background: rgba(255,255,255,0.1); border-radius: 20px; padding: 40px; text-align: center; backdrop-filter: blur(10px); max-width: 400px; width: 100%; }
        h1 { font-size: 2.5em; margin-bottom: 10px; color: #00d4ff; }
        .subtitle { font-size: 0.9em; color: #888; margin-bottom: 30px; }
        .level-meter { width: 100%; height: 30px; background: rgba(0,0,0,0.3); border-radius: 15px; overflow: hidden; margin-bottom: 30px; }
        .level-bar { height: 100%; width: 0%; background: linear-gradient(90deg, #00ff88, #00d4ff, #ffaa00, #ff4444); transition: width 0.1s; border-radius: 15px; }
        .mute-btn { padding: 20px 60px; font-size: 1.5em; border: none; border-radius: 15px; cursor: pointer; transition: all 0.3s; font-weight: bold; }
        .mute-btn.muted { background: #ff4444; color: white; }
        .mute-btn.unmuted { background: #00ff88; color: #1a1a2e; }
        .mute-btn:hover { transform: scale(1.05); }
        .status { margin-top: 20px; font-size: 0.8em; color: #888; }
        .connected { color: #00ff88; }
    </style>
</head>
<body>
    <div class="container">
        <h1>FAUNA</h1>
        <p class="subtitle">Audio Streaming Control</p>
        <div class="level-meter"><div class="level-bar" id="levelBar"></div></div>
        <button class="mute-btn unmuted" id="muteBtn">UNMUTED</button>
        <p class="status" id="status">Connecting...</p>
    </div>
    <script>
        var ws = null;
        var isMuted = false;
        var audioContext = null;
        var gainNode = null;
        
        function initAudio() {
            audioContext = new (window.AudioContext || window.webkitAudioContext)();
            gainNode = audioContext.createGain();
            gainNode.connect(audioContext.destination);
            gainNode.gain.value = 1.0;
        }
        
        function connect() {
            var wsUrl = "ws://" + window.location.host + "/audio";
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                document.getElementById("status").textContent = "Connected";
                document.getElementById("status").className = "status connected";
                if (!audioContext) initAudio();
            };
            
            ws.onclose = function() {
                document.getElementById("status").textContent = "Disconnected";
                setTimeout(connect, 2000);
            };
            
            ws.onerror = function() {
                document.getElementById("status").textContent = "Error";
            };
            
            ws.onmessage = function(event) {
                if (event.data instanceof Blob) {
                    event.data.arrayBuffer().then(function(buffer) {
                        if (audioContext) {
                            audioContext.decodeAudioData(buffer).then(function(audioBuffer) {
                                var source = audioContext.createBufferSource();
                                source.buffer = audioBuffer;
                                source.connect(gainNode);
                                source.start();
                            }).catch(function() {});
                        }
                    });
                }
            };
        }
        
        document.getElementById("muteBtn").addEventListener("click", function() {
            isMuted = !isMuted;
            if (isMuted) {
                this.textContent = "MUTED";
                this.className = "mute-btn muted";
                if (gainNode) gainNode.gain.value = 0;
            } else {
                this.textContent = "UNMUTED";
                this.className = "mute-btn unmuted";
                if (gainNode) gainNode.gain.value = 1;
            }
        });
        
        connect();
    </script>
</body>
</html>
)";
}

juce::String HTTPServer::buildHTTPResponse(const juce::String& request)
{
    if (request.startsWith("GET / ") || request.startsWith("GET /index.html") || request.startsWith("GET / HTTP") || request == "GET /")
    {
        juce::String htmlContent = getHTMLPage();
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + juce::String(htmlContent.length()) + "\r\n";
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
