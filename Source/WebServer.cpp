#include "WebServer.h"
#include <cstring>

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// FIX #2: Corrected SHA-1 implementation with proper padding and big-endian bit length
static void sha1(const char* data, int len, unsigned char* out)
{
    unsigned int h0 = 0x67452301;
    unsigned int h1 = 0xEFCDAB89;
    unsigned int h2 = 0x98BADCFE;
    unsigned int h3 = 0x10325476;
    unsigned int h4 = 0xC3D2E1F0;

    // Correct total length: must be multiple of 64
    int totalLen = len + 1 + 8;
    if (totalLen % 64 != 0)
        totalLen = ((totalLen / 64) + 1) * 64;

    juce::HeapBlock<unsigned char> buffer(totalLen, true); // zero-initialised
    std::memcpy(buffer.get(), data, len);
    buffer[len] = 0x80;

    // Write bit length as 64-bit big-endian at end of buffer
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
            w[j] = ((unsigned int)buffer[i + j * 4]     << 24) |
                   ((unsigned int)buffer[i + j * 4 + 1] << 16) |
                   ((unsigned int)buffer[i + j * 4 + 2] <<  8) |
                   ((unsigned int)buffer[i + j * 4 + 3]);
        }
        for (int j = 16; j < 80; j++)
        {
            unsigned int val = w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16];
            w[j] = (val << 1) | (val >> 31);
        }

        unsigned int a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int j = 0; j < 20; j++)
        {
            unsigned int f = (b & c) | ((~b) & d);
            unsigned int k = 0x5A827999;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        for (int j = 20; j < 40; j++)
        {
            unsigned int f = b ^ c ^ d;
            unsigned int k = 0x6ED9EBA1;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        for (int j = 40; j < 60; j++)
        {
            unsigned int f = (b & c) | (b & d) | (c & d);
            unsigned int k = 0x8F1BBCDC;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }
        for (int j = 60; j < 80; j++)
        {
            unsigned int f = b ^ c ^ d;
            unsigned int k = 0xCA62C1D6;
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d; d = c; c = (b << 30) | (b >> 2); b = a; a = temp;
        }

        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    out[0]  = (h0 >> 24) & 0xFF; out[1]  = (h0 >> 16) & 0xFF;
    out[2]  = (h0 >>  8) & 0xFF; out[3]  =  h0        & 0xFF;
    out[4]  = (h1 >> 24) & 0xFF; out[5]  = (h1 >> 16) & 0xFF;
    out[6]  = (h1 >>  8) & 0xFF; out[7]  =  h1        & 0xFF;
    out[8]  = (h2 >> 24) & 0xFF; out[9]  = (h2 >> 16) & 0xFF;
    out[10] = (h2 >>  8) & 0xFF; out[11] =  h2        & 0xFF;
    out[12] = (h3 >> 24) & 0xFF; out[13] = (h3 >> 16) & 0xFF;
    out[14] = (h3 >>  8) & 0xFF; out[15] =  h3        & 0xFF;
    out[16] = (h4 >> 24) & 0xFF; out[17] = (h4 >> 16) & 0xFF;
    out[18] = (h4 >>  8) & 0xFF; out[19] =  h4        & 0xFF;
}

// FIX #3: Corrected base64 encoding with proper padding
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

HTTPServer::HTTPServer()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    juce::String defaultIP = "127.0.0.1";
    localIP = defaultIP;
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
    OutputDebugString("FAUNA: Server started\n");
    return true;
}

void HTTPServer::stop()
{
    OutputDebugString("FAUNA: Stop requested\n");
    running = false;

    if (serverSocket != INVALID_SOCKET)
    {
        shutdown(serverSocket, SD_BOTH);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }

    if (serverThreadHandle != NULL)
    {
        OutputDebugString("FAUNA: Waiting for server thread...\n");
        DWORD waitResult = WaitForSingleObject(serverThreadHandle, 2000);
        if (waitResult == WAIT_TIMEOUT)
        {
            OutputDebugString("FAUNA: Thread timeout, terminating\n");
            TerminateThread(serverThreadHandle, 0);
        }
        else
        {
            OutputDebugString("FAUNA: Server thread exited cleanly\n");
        }
        CloseHandle(serverThreadHandle);
        serverThreadHandle = NULL;
    }

    {
        juce::ScopedLock lock(clientsLock);
        OutputDebugString(("FAUNA: Closing " + juce::String(audioClients.size()) + " client sockets\n").toUTF8());
        for (auto& client : audioClients)
        {
            if (client.socket != INVALID_SOCKET)
            {
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
        else
        {
            Sleep(10);
        }

        juce::ScopedLock lock(clientsLock);
        for (int i = audioClients.size() - 1; i >= 0; i--)
        {
            AudioClient& client = audioClients.getReference(i);
            if (client.socket != INVALID_SOCKET)
            {
                fd_set readSet;
                FD_ZERO(&readSet);
                FD_SET(client.socket, &readSet);
                TIMEVAL tv;
                tv.tv_sec = 0;
                tv.tv_usec = 50000;

                int result = select(0, &readSet, nullptr, nullptr, &tv);
                if (result > 0)
                {
                    handleWebSocketClient(client);
                }
                else if (result < 0)
                {
                    char dbg[128];
                    sprintf(dbg, "FAUNA: select error %d\n", WSAGetLastError());
                    OutputDebugString(dbg);
                }
            }
            if (client.socket == INVALID_SOCKET)
                audioClients.remove(i);
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

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    if (listen(listenSocket, 5) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    return listenSocket;
}

void HTTPServer::handleClient(SOCKET clientSocket)
{
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        juce::String request(buffer);
        OutputDebugString(("FAUNA: Full HTTP request:\n" + juce::String(buffer, bytesReceived) + "\n").toUTF8());
        OutputDebugString("FAUNA: HTTP request received, checking for WebSocket...\n");

        if (request.contains("Upgrade") && request.contains("websocket"))
        {
            OutputDebugString("FAUNA: Detected WebSocket upgrade request\n");

            const char* keyPtr = strstr(buffer, "Sec-WebSocket-Key:");
            juce::String wsKey = "dGhlIHNhbXBsZSBub25jZQ==";

            if (keyPtr != nullptr)
            {
                keyPtr += 18;
                while (*keyPtr == ' ') keyPtr++;
                const char* endPtr = strstr(keyPtr, "\r\n");
                if (endPtr != nullptr && endPtr > keyPtr)
                {
                    int keyLen = (int)(endPtr - keyPtr);
                    wsKey = juce::String(keyPtr, keyLen);
                }
            }

            OutputDebugString(("FAUNA: Parsed WebSocket key: " + wsKey + "\n").toUTF8());

            // FIX #1: Use std::string to avoid JUCE temp pointer lifetime issue
            juce::String acceptKey = generateWebSocketKey(wsKey.toStdString().c_str());
            OutputDebugString(("FAUNA: Generated accept key: " + acceptKey + "\n").toUTF8());

            juce::String response = "HTTP/1.1 101 Switching Protocols\r\n";
            response += "Upgrade: websocket\r\n";
            response += "Connection: Upgrade\r\n";
            response += "Sec-WebSocket-Accept: " + acceptKey + "\r\n";
            response += "\r\n";

            OutputDebugString("FAUNA: 101 Switching Protocols response ready to send\n");
            OutputDebugString(("FAUNA: Full response:\n" + response + "\n").toUTF8());
            OutputDebugString("FAUNA: Sending 101 Switching Protocols response...\n");
            int sent = send(clientSocket, response.toUTF8(), response.length(), 0);
            OutputDebugString(("FAUNA: 101 Response sent, bytes: " + juce::String(sent) + "\n").toUTF8());

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
                OutputDebugString(("FAUNA: Client IP: " + juce::String(ipStr) + "\n").toUTF8());
            }

            juce::ScopedLock lock(clientsLock);
            audioClients.add(client);
            OutputDebugString("FAUNA: Client added to list\n");

            // FIX #4: Removed direct handleWebSocketClient() call here.
            // The server loop's select() handles incoming frames correctly
            // and operates on the actual list entry (not a local copy).
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
        int err = WSAGetLastError();
        char dbg[128];
        sprintf(dbg, "FAUNA: recv error %d\n", err);
        OutputDebugString(dbg);
        closesocket(clientSocket);
    }
}

void HTTPServer::handleWebSocketClient(AudioClient& client)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(client.socket, &readSet);
    TIMEVAL tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    int ready = select(0, &readSet, nullptr, nullptr, &tv);
    if (ready <= 0)
        return;

    char buffer[8192];
    int bytesReceived = recv(client.socket, buffer, sizeof(buffer), 0);

    if (bytesReceived <= 0)
    {
        int err = WSAGetLastError();
        char dbg[128];
        sprintf(dbg, "FAUNA: recv returned %d, WSA error %d\n", bytesReceived, err);
        OutputDebugString(dbg);
        client.socket = INVALID_SOCKET;
        return;
    }

    char dbg[128];
    sprintf(dbg, "FAUNA: WS recv %d bytes, opcode=0x%02X, masked=%d, payloadLen=%u\n",
        bytesReceived, (unsigned char)buffer[0], (buffer[1] & 0x80) != 0, buffer[1] & 0x7F);
    OutputDebugString(dbg);

    unsigned char opcode = buffer[0] & 0x0F;
    bool masked = (buffer[1] & 0x80) != 0;
    unsigned int payloadLength = buffer[1] & 0x7F;

    int headerLen = 2;
    if (payloadLength == 126) headerLen += 2;
    if (payloadLength == 127) headerLen += 8;
    if (masked) headerLen += 4;

    if (opcode == 0x01) // Text frame
    {
        OutputDebugString("FAUNA: Processing text frame\n");
        if (bytesReceived >= headerLen)
        {
            char* data = buffer + headerLen;
            int dataLen = bytesReceived - headerLen;

            if (masked && dataLen > 0)
            {
                char* maskKey = buffer + headerLen - 4;
                for (int i = 0; i < dataLen; i++)
                    data[i] = data[i] ^ maskKey[i % 4];
            }

            juce::String message(data, dataLen);
            sprintf(dbg, "FAUNA: Text message: %s\n", message.toUTF8());
            OutputDebugString(dbg);
            if (message.contains("mute"))
                client.muted = message.contains("true");
        }
    }
    else if (opcode == 0x08) // Close frame
    {
        OutputDebugString("FAUNA: Close frame received, closing connection\n");
        sendWebSocketFrame(client.socket, nullptr, 0, 0x08);
        client.socket = INVALID_SOCKET;
    }
    else if (opcode == 0x09) // Ping frame
    {
        OutputDebugString("FAUNA: Ping received, sending pong\n");
        char pongPayload[4096];
        int pongLen = 0;

        if (payloadLength > 0 && payloadLength < 4096 && bytesReceived > headerLen)
        {
            pongLen = payloadLength;
            memcpy(pongPayload, buffer + headerLen, pongLen);

            if (masked)
            {
                char* maskKey = buffer + headerLen - 4;
                for (int i = 0; i < pongLen; i++)
                    pongPayload[i] = pongPayload[i] ^ maskKey[i % 4];
            }
        }

        sendWebSocketFrame(client.socket, pongLen > 0 ? pongPayload : nullptr, pongLen, 0x0A);
    }
    else if (opcode == 0x0A) // Pong frame
    {
        OutputDebugString("FAUNA: Pong received (connection alive)\n");
    }
}

// FIX #1: Use std::string to avoid JUCE temporary pointer lifetime issues
juce::String HTTPServer::generateWebSocketKey(const char* key)
{
    std::string combined = std::string(key) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    unsigned char hash[20];
    sha1(combined.c_str(), (int)combined.size(), hash);

    return base64Encode(hash, 20);
}

void HTTPServer::sendWebSocketFrame(SOCKET socket, const char* data, int length, int opcode)
{
    if (socket == INVALID_SOCKET)
        return;

    char dbg[128];
    sprintf(dbg, "FAUNA: Sending WS frame: opcode=0x%02X, length=%d\n", opcode, length);
    OutputDebugString(dbg);

    char header[14];
    int headerLen = 2;
    header[0] = (char)(0x80 | (opcode & 0x0F)); // FIN bit set, no mask on server frames (RFC 6455)

    if (length == 0)
    {
        header[1] = 0;
    }
    else if (length < 126)
    {
        header[1] = (char)(length & 0x7F);
    }
    else if (length < 65536)
    {
        header[1] = 126;
        header[2] = (char)((length >> 8) & 0xFF);
        header[3] = (char)(length & 0xFF);
        headerLen = 4;
    }
    else
    {
        header[1] = 127;
        for (int i = 0; i < 8; i++)
            header[2 + i] = 0;
        header[10] = (char)((length >> 8) & 0xFF);
        header[11] = (char)(length & 0xFF);
        headerLen = 12;
    }

    send(socket, header, headerLen, 0);
    if (length > 0 && data != nullptr)
        send(socket, data, length, 0);
}

void HTTPServer::broadcastAudio(const float* audioData, int numSamples)
{
}

void HTTPServer::writeAudioData(const float* audioData, int numSamples)
{
    broadcastAudio(audioData, numSamples);
}

juce::String HTTPServer::getHTMLPage()
{
    juce::String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    html += "<title>FAUNA Test</title>";
    html += "<style>";
    html += "body{font-family:Arial;background:#1a1a2e;color:white;min-height:100vh;margin:0;display:flex;align-items:center;justify-content:center}";
    html += ".c{background:rgba(255,255,255,0.1);padding:40px;border-radius:20px;text-align:center;max-width:500px}";
    html += "h1{color:#00d4ff}";
    html += "p{margin:15px 0}";
    html += ".ok{color:#0f0}";
    html += ".err{color:#f00}";
    html += ".log{text-align:left;font-size:12px;color:#666;max-height:150px;overflow:auto;border:1px solid #333;padding:10px;margin-top:20px}";
    html += "</style></head><body>";
    html += "<div class=\"c\">";
    html += "<h1>FAUNA</h1>";
    html += "<p>WebSocket Connection Test</p>";
    html += "<p id=\"s\" class=\"err\">Status: Click button to test</p>";
    html += "<button onclick=\"testWS()\" style=\"padding:10px 20px;font-size:16px;cursor:pointer\">Test WebSocket</button>";
    html += "<div class=\"log\" id=\"l\"></div>";
    html += "</div>";
    html += "<script>";
    html += "var d=document.getElementById('l'),s=document.getElementById('s');";
    html += "function log(m){d.innerHTML+=m+'<br>';d.scrollTop=d.scrollHeight;console.log(m);}";
    html += "function testWS(){";
    html += "log('Connecting to ws://'+location.host);";
    html += "var ws=new WebSocket('ws://'+location.host);";
    html += "ws.onopen=function(){log('OPEN - Connected!');s.textContent='Connected!';s.className='ok';};";
    html += "ws.onclose=function(e){log('CLOSE code:'+e.code);s.textContent='Disconnected';s.className='err';};";
    html += "ws.onerror=function(){log('ERROR');s.textContent='Error';};";
    html += "ws.onmessage=function(e){log('MSG:'+e.data);};";
    html += "}";
    html += "log('Ready. Click button to test.');";
    html += "</script></body></html>";
    return html;
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
