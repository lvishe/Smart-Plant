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
    
    // Public so Server.ino can use them
    bool receivePacket(Opcode &codeOut, String &strOut, String &fromIP);
    bool sendUpdate(int newDelay);
};

#endif