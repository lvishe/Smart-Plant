#ifndef LCLIENT_H
#define LCLIENT_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"

class Lclient {
private:
    WiFiUDP _udp;
    IPAddress _serverIP;
    int _port;

public:
    Lclient(const char* serverIP = "192.168.4.1", int port = 8080);
    void begin();
    bool sendPacket(ClientMessage &msg);
    bool receivePacket(ServerMessage &msgOut);
};

#endif