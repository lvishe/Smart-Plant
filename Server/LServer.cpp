#include "LServer.h"
#include <Arduino.h>

void Lserver::startAP() 
{   
<<<<<<< HEAD
    WiFi.softAP("ESP32_AP");
=======
    WiFi.softAP("ESP32_AP", "12345678");
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
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

<<<<<<< HEAD
bool Lserver::receivePacket(ClientMessage &msgOut, String &fromIP) 
{
    // Check if we have enough bytes for a complete sensor reading
    if (!_client.connected() || _client.available() < sizeof(ClientMessage)) 
=======
// Updated to include String &fromIP
bool Lserver::receivePacket(Opcode &codeOut, String &strOut, String &fromIP) 
{
    if (!_client.connected() || _client.available() < sizeof(Message)) 
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
    {
        return false;
    }

<<<<<<< HEAD
    fromIP = _client.remoteIP().toString();

    // Read the raw bytes directly into the struct's memory
    // readBytes is great because it handles minor network timing delays natively
    size_t bytesRead = _client.readBytes((char*)&msgOut, sizeof(ClientMessage));
    
    return bytesRead == sizeof(ClientMessage);
}

bool Lserver::sendUpdate(uint32_t newDelay) 
=======
    // Capture the IP address of the sender
    fromIP = _client.remoteIP().toString();

    // 1. Read the Header
    Message header;
    _client.read((uint8_t*)&header, sizeof(Message));
    codeOut = header.code;

    // 2. Wait for Payload
    unsigned long start = millis();
    while (_client.available() < header.data_lenght) 
    {
        if (millis() - start > 1000) 
        {
            return false;
        }
    }

    // 3. Read the Data
    char* buf = new char[header.data_lenght + 1];
    _client.read((uint8_t*)buf, header.data_lenght);
    buf[header.data_lenght] = '\0';
    strOut = String(buf);
    delete[] buf;
    
    return true;
}

bool Lserver::sendUpdate(int newDelay) 
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
{
    if (!_client.connected()) 
    {
        return false;
    }

<<<<<<< HEAD
    // 1. Create and populate the packed struct
    ServerMessage msg;
    msg.type = MessageType::SetDelay;
    msg.delay_ms = newDelay;

    // 2. Send the struct in one single blast
    size_t sent = _client.write((uint8_t*)&msg, sizeof(ServerMessage));
    
    return sent == sizeof(ServerMessage);
=======
    // 1. Prepare Header
    Message header;
    header.code = Opcode::UPDATE;
    header.data_lenght = sizeof(int);

    // 2. Bundle into ONE buffer
    int totalSize = sizeof(Message) + sizeof(int);
    uint8_t packet[totalSize]; 

    memcpy(packet, &header, sizeof(Message));
    memcpy(packet + sizeof(Message), &newDelay, sizeof(int));

    // 3. Send as one single packet
    return _client.write(packet, totalSize) == totalSize;
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
}