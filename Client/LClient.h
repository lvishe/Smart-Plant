#ifndef LCLIENT_H
#define LCLIENT_H

#include <WiFi.h>
#include "protocol.h"

class Lclient {
private:
    WiFiClient _client;

public:
    void startSTA();
    
    // Sends Header + Data in one single buffer
    bool sendPacket(String& data, Opcode code);
    
    // Reads one full packet
    bool receivePacket(Opcode &codeOut, String &strOut, int &intOut);
};

#endif