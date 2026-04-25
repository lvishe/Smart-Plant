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
    
<<<<<<< HEAD
    // Now we just pass the struct by reference to populate it
    bool receivePacket(ClientMessage &msgOut, String &fromIP);
    bool sendUpdate(uint32_t newDelay); 
=======
    // Public so Server.ino can use them
    bool receivePacket(Opcode &codeOut, String &strOut, String &fromIP);
    bool sendUpdate(int newDelay);
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
};

#endif