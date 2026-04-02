#include "WebServer.h"

WebServer::WebServer()
{
    detectLocalIP();
}

WebServer::~WebServer()
{
    stop();
}

void WebServer::detectLocalIP()
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

bool WebServer::start(int targetPort)
{
    port = targetPort;
    running = true;
    return true;
}

void WebServer::stop()
{
    running = false;
}
