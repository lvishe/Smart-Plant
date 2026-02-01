#include "Server.h"

void Lserver::startAP(const char* ssid, const char* password) 
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    _server.begin();
    Serial.print("Server started. IP: ");
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

int Lserver::readOpcode(PacketHeader &headerOut)
{
    if (_client && _client.connected() && _client.available() >= sizeof(PacketHeader))
    {
        _client.read((uint8_t*)&headerOut, sizeof(PacketHeader));
        return (int)headerOut.code;
    }
    return -1;
}

String Lserver::readStringPayload(int length)
{
    // Wait for full payload (with timeout)
    unsigned long start = millis();
    while (_client.available() < length)
    {
        if (millis() - start > 1000) return "";
    }
    
    char* buffer = new char[length + 1];
    _client.read((uint8_t*)buffer, length);
    buffer[length] = '\0';
    String res = String(buffer);
    delete[] buffer;
    return res;
}

void Lserver::sendResponse(int delayTime)
{
    PacketHeader header;
    header.code = Opcode::UPDATE;
    header.data_length = sizeof(int);
    
    _client.write((uint8_t*)&header, sizeof(header));
    _client.write((uint8_t*)&delayTime, sizeof(delayTime));
}