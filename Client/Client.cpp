#include "Client.h"

void Lclient::startSTA(const char* ssid, const char* password, const char* host, int port) 
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    
    if (_client.connect(host, port)) Serial.println("Connected");
}

int Lclient::performTransaction(const String& message, int currentDelay)
{
    if (!_client.connected()) return currentDelay;

    // 1. Send Request (DATA)
    PacketHeader reqHeader;
    reqHeader.code = Opcode::DATA;
    reqHeader.data_length = message.length();
    
    _client.write((uint8_t*)&reqHeader, sizeof(reqHeader));
    _client.print(message);

    // 2. Wait for Response (Timeout 2000ms)
    unsigned long start = millis();
    while (_client.available() < sizeof(PacketHeader))
    {
        if (millis() - start > 2000) {
            Serial.println("Timeout waiting for server response");
            return currentDelay;
        }
    }

    // 3. Read Response Header
    PacketHeader respHeader;
    _client.read((uint8_t*)&respHeader, sizeof(PacketHeader));

    // 4. Switch Case for Parsing Response
    switch (respHeader.code)
    {
        case Opcode::UPDATE:
        {
            // Verify payload size
            if (respHeader.data_length == sizeof(int))
            {
                int newDelay;
                _client.read((uint8_t*)&newDelay, sizeof(int));
                return newDelay; // Return new delay from server
            }
            break;
        }
        case Opcode::DATA:
            // Client shouldn't receive DATA from server
            break;
    }

    // Clear buffer if format was wrong
    while(_client.available()) _client.read();
    
    return currentDelay;
}