#include "LClient.h"

void Lclient::startSTA() {
    WiFi.begin("ESP32_AP");
    while (WiFi.status() != WL_CONNECTED) delay(500);
    _client.connect("192.168.4.1", 8080);
}

bool Lclient::sendPacket(ClientMessage &msg)
{
    if (!_client.connected()) 
    {
        return false;
    }
    
    // No dynamic allocation (new/delete) required anymore!
    // Send the memory block containing the struct directly.
    size_t sent = _client.write((uint8_t*)&msg, sizeof(ClientMessage));
    
    return sent == sizeof(ClientMessage);
}

bool Lclient::receivePacket(ServerMessage &msgOut)
{
    // Wait until a full ServerMessage has arrived
    if (!_client.connected() || _client.available() < sizeof(ServerMessage)) 
    {
        return false;
    }

    // Read the payload directly into the struct
    size_t bytesRead = _client.readBytes((char*)&msgOut, sizeof(ServerMessage));
    
    return bytesRead == sizeof(ServerMessage);
}