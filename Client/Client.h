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
    inline Lserver(int port = 8080) : _server(port) {}
    
    void startAP(const char* ssid = "ESP32_AP", const char* password = "password123");
    bool acceptClient();
    
    // Reads header and returns the Opcode. Returns -1 if no data.
    int readOpcode(PacketHeader &headerOut);
    
    // Reads the specific payload types
    String readStringPayload(int length);
    
    // Sends the response back to the waiting client
    void sendResponse(int delayTime);
};

#endif