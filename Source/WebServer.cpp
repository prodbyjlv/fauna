#include "WebServer.h"

WebServer::WebServer()
{
}

WebServer::~WebServer()
{
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

int WebServer::getConnectedClients() const
{
    return 0;
}

void WebServer::setMuteState(int clientId, bool muted)
{
}

void WebServer::writeAudioData(const float* audioData, int numSamples)
{
}

float WebServer::getCurrentLevel() const
{
    return 0.0f;
}
