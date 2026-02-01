#ifndef SERVER_H
#define SERVER_H

#include <WiFi.h>
#include "protocol.h"

class Lserver 
{
private:
    WiFiServer _server;
    WiFiClient _client;

public:
    inline Lserver(int port = 8080) : _server(port) {} // Default port usually 80 or 8080
    
    void startAP(const char* ssid = "ESP32_AP", const char* password = "password123");
    bool acceptClient();
    
    // Returns true if a full message was parsed
    bool receive(Opcode &opOut, String &dataOut);
    
    // Send an update command to the client
    void sendUpdate(int newDelay);
};

#endif