#ifndef LSERVER_H
#define LSERVER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "protocol.h"

class Lserver 
{
private:
    WiFiUDP _udp;
    int _port;
    
    IPAddress _lastSenderIP;
    uint16_t _lastSenderPort;

public:
    Lserver(int port = 8080) : _port(port) {}

    /** Start UDP; call only after the sketch has configured WiFi (e.g. softAP/STA). */
    void begin();
    // Returns true if a valid packet was received
    bool receivePacket(ClientMessage &msgOut, String &fromIP);
    // Sends a reply to the last client that sent a packet
    bool sendUpdate(uint32_t newDelay); 
};

#endif
