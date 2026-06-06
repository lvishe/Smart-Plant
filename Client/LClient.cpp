#include "LClient.h"

Lclient::Lclient(const char* serverIP, int port) 
{
    _serverIP.fromString(serverIP);
    _port = port;
}

void Lclient::begin() 
{
    WiFi.begin("SmartPlant");
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    _udp.begin(_port); // Listen on the same port for replies
}

bool Lclient::sendPacket(ClientMessage &msg) 
{
    _udp.beginPacket(_serverIP, _port);
    _udp.write((uint8_t*)&msg, sizeof(ClientMessage));
    return _udp.endPacket();
}

bool Lclient::receivePacket(ServerMessage &msgOut) 
{
    int packetSize = _udp.parsePacket();
    if (packetSize == sizeof(ServerMessage)) 
    {
        _udp.read((char*)&msgOut, sizeof(ServerMessage));
        return true;
    }

    return false;
}