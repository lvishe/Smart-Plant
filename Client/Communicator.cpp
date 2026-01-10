#include "Communicator.h"

void Lclient::startSTA(const char* ssid, const char* password, const char* host, int port) 
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to AP");
    
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.println("Connected to AP");

    if (!_client.connect(host, port)) 
    {
        Serial.println("Connection failed");
    } 
    else 
    {
        Serial.println("Connected to receiver");
    }
}

void Lclient::send(const String& message)
{
    if (_client.connected())
    {
        _client.println(message);
    }
}

bool Lclient::connected()
{
    return _client.connected();
}
