#include "WebServer.h"
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <iomanip>
#include <sstream>

WebServer::WebServer()
{
    buildWebPage();
}

WebServer::~WebServer()
{
    stop();
}

void WebServer::buildWebPage()
{
    webPageContent = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FAUNA Audio Stream</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            color: white;
            padding: 20px;
        }
        .container {
            background: rgba(255,255,255,0.1);
            border-radius: 20px;
            padding: 40px;
            text-align: center;
            backdrop-filter: blur(10px);
            max-width: 400px;
            width: 100%;
        }
        h1 {
            font-size: 2em;
            margin-bottom: 10px;
            color: #00d4ff;
        }
        .status {
            font-size: 0.9em;
            color: #aaa;
            margin-bottom: 30px;
        }
        .level-meter {
            width: 100%;
            height: 30px;
            background: rgba(0,0,0,0.3);
            border-radius: 15px;
            overflow: hidden;
            margin-bottom: 30px;
        }
        .level-bar {
            height: 100%;
            width: 0%;
            background: linear-gradient(90deg, #00ff88, #00d4ff, #ffaa00, #ff4444);
            transition: width 0.1s;
            border-radius: 15px;
        }
        .mute-btn {
            padding: 20px 60px;
            font-size: 1.5em;
            border: none;
            border-radius: 15px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
        }
        .mute-btn.muted {
            background: #ff4444;
            color: white;
        }
        .mute-btn.unmuted {
            background: #00ff88;
            color: #1a1a2e;
        }
        .mute-btn:hover {
            transform: scale(1.05);
        }
        .connection-status {
            margin-top: 20px;
            font-size: 0.8em;
            color: #888;
        }
        .connected {
            color: #00ff88;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>FAUNA</h1>
        <p class="status">Audio Streaming Control</p>
        
        <div class="level-meter">
            <div class="level-bar" id="levelBar"></div>
        </div>
        
        <button class="mute-btn unmuted" id="muteBtn" onclick="toggleMute()">UNMUTED</button>
        
        <p class="connection-status" id="status">Connecting...</p>
    </div>

    <script>
        let ws = null;
        let isMuted = false;
        let isConnected = false;

        function connect() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = protocol + '//' + window.location.host + '/audio';
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                isConnected = true;
                document.getElementById('status').textContent = 'Connected';
                document.getElementById('status').className = 'connection-status connected';
            };
            
            ws.onclose = function() {
                isConnected = false;
                document.getElementById('status').textContent = 'Disconnected - Reconnecting...';
                document.getElementById('status').className = 'connection-status';
                setTimeout(connect, 2000);
            };
            
            ws.onerror = function() {
                document.getElementById('status').textContent = 'Connection Error';
            };
            
            ws.onmessage = function(event) {
                if (typeof event.data === 'string') {
                    const msg = JSON.parse(event.data);
                    if (msg.type === 'level') {
                        document.getElementById('levelBar').style.width = (msg.value * 100) + '%';
                    }
                }
            };
        }

        function toggleMute() {
            isMuted = !isMuted;
            const btn = document.getElementById('muteBtn');
            if (isMuted) {
                btn.textContent = 'MUTED';
                btn.className = 'mute-btn muted';
            } else {
                btn.textContent = 'UNMUTED';
                btn.className = 'mute-btn unmuted';
            }
            
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: 'mute', value: isMuted }));
            }
        }

        connect();
    </script>
</body>
</html>
)";
}

juce::String WebServer::generateWebSocketHandshake(const juce::String& key)
{
    juce::String acceptKey = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    unsigned char hash[20];
    SHA1((unsigned char*)acceptKey.toUTF8(), acceptKey.length(), hash);
    
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, hash, 20);
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    juce::String result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

bool WebServer::start(int port)
{
    this->port = port;
    
    serverSocket = new juce::StreamingSocket();
    
    if (serverSocket->bind(port))
    {
        running = true;
        startThread();
        return true;
    }
    
    delete serverSocket;
    serverSocket = nullptr;
    return false;
}

void WebServer::stop()
{
    running = false;
    
    if (serverSocket)
    {
        serverSocket->close();
        delete serverSocket;
        serverSocket = nullptr;
    }
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (auto& conn : connections)
        {
            if (conn.second.socket)
            {
                conn.second.socket->close();
                delete conn.second.socket;
            }
        }
        connections.clear();
    }
    
    stopThread(500);
}

juce::String WebServer::getLocalIPAddress()
{
    juce::Array<juce::IPAddress> addresses;
    juce::IPAddress::findAllAddresses(addresses);
    
    for (auto& addr : addresses)
    {
        juce::String ip = addr.toString();
        if (ip.startsWith("192.168.") || ip.startsWith("10.") || ip.startsWith("172."))
        {
            return ip;
        }
    }
    
    if (addresses.size() > 0)
        return addresses[0].toString();
    
    return "127.0.0.1";
}

void WebServer::run()
{
    while (running && !threadShouldExit())
    {
        if (serverSocket && serverSocket->isConnected())
        {
            juce::StreamingSocket* clientSocket = serverSocket->waitForNextConnection();
            
            if (clientSocket)
            {
                handleClient(clientSocket);
            }
        }
        
        juce::Thread::sleep(10);
    }
}

void WebServer::handleClient(juce::StreamingSocket* clientSocket)
{
    char buffer[8192];
    int bytesRead = clientSocket->read(buffer, sizeof(buffer) - 1, false);
    
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        juce::String request(buffer);
        
        if (request.contains("Upgrade: websocket"))
        {
            juce::String response = handleHTTPRequest(request, clientSocket);
            clientSocket->write(response.toUTF8(), response.length());
            
            int clientId = nextClientId++;
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                connections[clientId].socket = clientSocket;
                connections[clientId].isWebSocket = true;
                connections[clientId].muted = false;
                connections[clientId].ipAddress = clientSocket->getHostName();
            }
            connectedClients++;
            
            char wsBuffer[4096];
            while (running && clientSocket->isConnected())
            {
                int wsBytes = clientSocket->read(wsBuffer, sizeof(wsBuffer) - 1, true);
                if (wsBytes > 0)
                {
                    handleWebSocketFrame(clientSocket, wsBuffer, wsBytes);
                }
                juce::Thread::sleep(1);
            }
            
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                if (connections.find(clientId) != connections.end())
                {
                    connections.erase(clientId);
                }
            }
            connectedClients--;
        }
        else
        {
            juce::String response = handleHTTPRequest(request, clientSocket);
            clientSocket->write(response.toUTF8(), response.length());
            clientSocket->close();
            delete clientSocket;
        }
    }
    else
    {
        clientSocket->close();
        delete clientSocket;
    }
}

juce::String WebServer::handleHTTPRequest(const juce::String& request, juce::StreamingSocket* socket)
{
    juce::String response;
    
    if (request.startsWith("GET / "))
    {
        juce::String httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += "Content-Type: text/html\r\n";
        httpResponse += "Content-Length: " + juce::String(webPageContent.length()) + "\r\n";
        httpResponse += "Connection: close\r\n";
        httpResponse += "\r\n";
        httpResponse += webPageContent;
        return httpResponse;
    }
    else if (request.startsWith("GET /status "))
    {
        juce::String status = "{\"clients\":" + juce::String(getConnectedClients()) + 
                            ",\"port\":" + juce::String(port) + 
                            ",\"ip\":\"" + getLocalIPAddress() + "\"}";
        
        juce::String httpResponse = "HTTP/1.1 200 OK\r\n";
        httpResponse += "Content-Type: application/json\r\n";
        httpResponse += "Content-Length: " + juce::String(status.length()) + "\r\n";
        httpResponse += "Access-Control-Allow-Origin: *\r\n";
        httpResponse += "Connection: close\r\n";
        httpResponse += "\r\n";
        httpResponse += status;
        return httpResponse;
    }
    else
    {
        juce::String body = "<html><body><h1>404 Not Found</h1></body></html>";
        juce::String httpResponse = "HTTP/1.1 404 Not Found\r\n";
        httpResponse += "Content-Type: text/html\r\n";
        httpResponse += "Content-Length: " + juce::String(body.length()) + "\r\n";
        httpResponse += "Connection: close\r\n";
        httpResponse += "\r\n";
        httpResponse += body;
        return httpResponse;
    }
    
    return response;
}

void WebServer::handleWebSocketFrame(juce::StreamingSocket* socket, const char* data, int length)
{
    if (length < 2) return;
    
    unsigned char opcode = data[0] & 0x0F;
    unsigned char lengthByte = data[1] & 0x7F;
    
    if (opcode == 0x08)
    {
        socket->close();
        return;
    }
    
    if (opcode == 0x01)
    {
        int headerLength = 2;
        if (lengthByte == 126) headerLength = 4;
        if (lengthByte == 127) headerLength = 10;
        
        if (length > headerLength)
        {
            juce::String message(data + headerLength, length - headerLength);
            
            if (message.contains("\"mute\":true"))
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                for (auto& conn : connections)
                {
                    if (conn.second.socket == socket)
                    {
                        conn.second.muted = true;
                        break;
                    }
                }
            }
            else if (message.contains("\"mute\":false"))
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                for (auto& conn : connections)
                {
                    if (conn.second.socket == socket)
                    {
                        conn.second.muted = false;
                        break;
                    }
                }
            }
        }
    }
}

int WebServer::getConnectedClients() const
{
    return connectedClients.load();
}

void WebServer::setMuteState(int clientId, bool muted)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    if (connections.find(clientId) != connections.end())
    {
        connections[clientId].muted = muted;
    }
}

void WebServer::writeAudioData(const float* audioData, int numSamples)
{
    juce::String levelMsg = "{\"type\":\"level\",\"value\":" + juce::String(getCurrentLevel()) + "}";
    
    std::lock_guard<std::mutex> lock(connectionsMutex);
    for (auto& conn : connections)
    {
        if (conn.second.socket && conn.second.socket->isConnected() && !conn.second.muted)
        {
            juce::MemoryBlock block;
            block.setSize(numSamples * 2 * sizeof(float));
            memcpy(block.getData(), audioData, numSamples * 2 * sizeof(float));
        }
    }
}
