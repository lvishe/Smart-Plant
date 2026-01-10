#ifndef COMM_H
#define COMM_H

#include "..\\common\\WIFI_DATA.h"

#include <WiFi.h>

class Lclient 
{
private:
    WiFiClient _client;

public:
    Lclient() = default;
    
    void startSTA(const char* ssid = WIFI_SSID , const char* password = WIFI_PASSWORD, const char* host = WIFI_HOST, int port = WIFI_PORT);
    
    void send(const String& message);
    
    bool connected();
};

#endif
