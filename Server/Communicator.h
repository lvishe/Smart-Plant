#ifndef COMM_H
#define COMM_H

#include "..\\common\\WIFI_DATA.h"

#include <WiFi.h>

class Lserver 
{
private:
    WiFiServer _server;
    WiFiClient _client;

public:
    inline Lserver(int port = WIFI_PORT) : _server(port) {}
    
    void startAP(const char* ssid = WIFI_SSID , const char* password = WIFI_PASSWORD);
  
    bool receive(String& response_out);  
    bool connected();
    bool acceptClient();
};

#endif
