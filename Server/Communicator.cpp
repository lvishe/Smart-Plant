#include "Communicator.h"

void Lserver::startAP(const char* ssid, const char* password) 
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);;
    
    _server.begin();
    Serial.print("Server started\n");
    Serial.print("AP IP: ");
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
            Serial.println("Client accepted");
            return true;
        }
    }
    return false;
}


bool Lserver::receive(String& response_out)
{
    if (_client && _client.connected() && _client.available())
    {
        response_out = _client.readStringUntil('\n');
        return true;
    }
    return false;
}
