#include "LServer.h"

void Lserver::begin() {   
    WiFi.softAP("Smart-Plant"); 
    _udp.begin(_port);
    Serial.print("UDP Server started on IP: ");
    Serial.print(WiFi.softAPIP());
    Serial.print(" Port: ");
    Serial.println(_port);
}

bool Lserver::receivePacket(ClientMessage &msgOut, String &fromIP) 
{
    
    int packetSize = _udp.parsePacket();

    if (packetSize == sizeof(ClientMessage))
    {
        
        _lastSenderIP = _udp.remoteIP();
        _lastSenderPort = _udp.remotePort();
        fromIP = _lastSenderIP.toString();

        _udp.read((char*)&msgOut, sizeof(ClientMessage));
        return true;
    }

    return false;
}

bool Lserver::sendUpdate(uint32_t newDelay) 
{
    if (_lastSenderIP[0] == 0) return false; // No one to reply to

    ServerMessage msg;
    msg.type = MessageType::SetDelay;
    msg.data = newDelay;

    _udp.beginPacket(_lastSenderIP, _lastSenderPort);
    _udp.write((uint8_t*)&msg, sizeof(ServerMessage));
    return _udp.endPacket();
}