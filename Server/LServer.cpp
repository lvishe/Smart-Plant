#include "LServer.h"
#include <Arduino.h>

void Lserver::startAP() 
{   
    WiFi.softAP("ESP32_AP");
    _server.begin();
    Serial.print("Server IP: ");
    Serial.println(WiFi.softAPIP());
}

bool Lserver::acceptClient() 
{
    if (!_client || !_client.connected()) 
    {
        WiFiClient newClient = _server.available();
        if (newClient) 
        {
            _client = newClient;
            return true;
        }
    }
   
    return false;
}

bool Lserver::receivePacket(ClientMessage &msgOut, String &fromIP) 
{
    // Check if we have enough bytes for a complete sensor reading
    if (!_client.connected() || _client.available() < sizeof(ClientMessage)) 
    {
        return false;
    }

    fromIP = _client.remoteIP().toString();

    // Read the raw bytes directly into the struct's memory
    // readBytes is great because it handles minor network timing delays natively
    size_t bytesRead = _client.readBytes((char*)&msgOut, sizeof(ClientMessage));
    
    return bytesRead == sizeof(ClientMessage);
}

bool Lserver::sendUpdate(uint32_t newDelay) 
{
    if (!_client.connected()) 
    {
        return false;
    }

    // 1. Create and populate the packed struct
    ServerMessage msg;
    msg.type = MessageType::SetDelay;
    msg.delay_ms = newDelay;

    // 2. Send the struct in one single blast
    size_t sent = _client.write((uint8_t*)&msg, sizeof(ServerMessage));
    
    return sent == sizeof(ServerMessage);
}