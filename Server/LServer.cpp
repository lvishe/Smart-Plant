#include "LServer.h"
#include <Arduino.h>

void Lserver::startAP() 
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_AP", "12345678");
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

// Updated to include String &fromIP
bool Lserver::receivePacket(Opcode &codeOut, String &strOut, String &fromIP) 
{
    if (!_client.connected() || _client.available() < sizeof(Message)) 
    {
        return false;
    }

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
{
    if (!_client.connected()) 
    {
        return false;
    }

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
}