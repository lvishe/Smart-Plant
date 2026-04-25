#ifndef LSERVER_H
#define LSERVER_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "protocol.h"

class Lserver 
{
private:
    WiFiServer _server;
    WiFiClient _client;

public:
    Lserver(int port = 8080) : _server(port) {}
    
    void startAP();
    bool acceptClient();
    
    // Now we just pass the struct by reference to populate it
    bool receivePacket(ClientMessage &msgOut, String &fromIP);
    bool sendUpdate(uint32_t newDelay); 
};

#endif