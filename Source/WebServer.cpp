#include "WebServer.h"

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
    {
        localIP = addresses[0].toString();
    }
}

bool HTTPServer::start(int targetPort)
{
    port = targetPort;
    
    serverSocket = createServerSocket(port);
    if (serverSocket == INVALID_SOCKET)
    {
        return false;
    }
    
    running = true;
    return true;
}

SOCKET HTTPServer::createServerSocket(int port)
{
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
    }
    
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

void HTTPServer::stop()
{
    running = false;
    
    if (serverSocket != INVALID_SOCKET)
    {
        shutdown(serverSocket, SD_BOTH);
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    for (auto& client : clients)
    {
        if (client.socket != INVALID_SOCKET)
        {
            shutdown(client.socket, SD_BOTH);
            closesocket(client.socket);
        }
    }
    clients.clear();
}

void HTTPServer::processConnections()
{
    if (!running || serverSocket == INVALID_SOCKET)
        return;
    
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(serverSocket, &readSet);
    
    TIMEVAL timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    
    int result = select(0, &readSet, nullptr, nullptr, &timeout);
    
    if (result > 0 && FD_ISSET(serverSocket, &readSet))
    {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket != INVALID_SOCKET)
        {
            handleClient(clientSocket);
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
        
        juce::String response = buildHTTPResponse(request);
        send(clientSocket, response.toUTF8(), response.length(), 0);
    }
    
    char checkBuf[1];
    int peekResult = recv(clientSocket, checkBuf, 1, MSG_PEEK);
    
    if (peekResult == 0 || peekResult == SOCKET_ERROR)
    {
        closesocket(clientSocket);
    }
    else
    {
        HTTPClient client;
        client.socket = clientSocket;
        client.muted = false;
        
        sockaddr_in addr;
        int addrLen = sizeof(addr);
        if (getpeername(clientSocket, (sockaddr*)&addr, &addrLen) == 0)
        {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ipStr, INET_ADDRSTRLEN);
            client.ipAddress = ipStr;
        }
        
        clients.add(client);
    }
}

juce::String HTTPServer::getHTMLPage()
{
    juce::String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FAUNA Audio Stream</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
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
        h1 { font-size: 2.5em; margin-bottom: 10px; color: #00d4ff; }
        .subtitle { font-size: 0.9em; color: #888; margin-bottom: 30px; }
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
        .mute-btn.muted { background: #ff4444; color: white; }
        .mute-btn.unmuted { background: #00ff88; color: #1a1a2e; }
        .mute-btn:hover { transform: scale(1.05); }
        .status { margin-top: 20px; font-size: 0.8em; color: #888; }
    </style>
</head>
<body>
    <div class="container">
        <h1>FAUNA</h1>
        <p class="subtitle">Audio Streaming Control</p>
        <div class="level-meter">
            <div class="level-bar" id="levelBar"></div>
        </div>
        <button class="mute-btn unmuted" id="muteBtn">UNMUTED</button>
        <p class="status" id="status">Connected</p>
    </div>
    <script>
        var isMuted = false;
        var level = 0;
        
        var muteBtn = document.getElementById("muteBtn");
        muteBtn.addEventListener("click", function() {
            isMuted = !isMuted;
            if (isMuted) {
                muteBtn.textContent = "MUTED";
                muteBtn.className = "mute-btn muted";
                document.getElementById("status").textContent = "Muted";
            } else {
                muteBtn.textContent = "UNMUTED";
                muteBtn.className = "mute-btn unmuted";
                document.getElementById("status").textContent = "Playing";
            }
        });
        
        setInterval(function() {
            level = Math.random();
            document.getElementById("levelBar").style.width = (level * 100) + "%";
        }, 100);
    </script>
</body>
</html>
)";
    return html;
}

juce::String HTTPServer::buildHTTPResponse(const juce::String& request)
{
    if (request.startsWith("GET / ") || request.startsWith("GET /index.html") || request.startsWith("GET / HTTP"))
    {
        juce::String htmlContent = getHTMLPage();
        
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + juce::String(htmlContent.length()) + "\r\n";
        response += "Connection: close\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "\r\n";
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
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += status;
        return response;
    }
    else if (request.startsWith("GET /ip"))
    {
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/plain\r\n";
        response += "Content-Length: " + juce::String(localIP.length()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += localIP;
        return response;
    }
    else
    {
        juce::String body = "<html><body><h1>FAUNA</h1><p>Audio Streaming Plugin</p><p><a href='/'>Home</a></p></body></html>";
        juce::String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + juce::String(body.length()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += body;
        return response;
    }
}
