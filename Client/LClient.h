#ifndef LCLIENT_H
#define LCLIENT_H

#include <WiFi.h>
#include "protocol.h"

class Lclient {
private:
    WiFiClient _client;

public:
    void startSTA();
    
<<<<<<< HEAD
    // Pass the struct directly
    bool sendPacket(ClientMessage &msg);
    
    // Read directly into the struct
    bool receivePacket(ServerMessage &msgOut);
=======
    // Sends Header + Data in one single buffer
    bool sendPacket(String& data, Opcode code);
    
    // Reads one full packet
    bool receivePacket(Opcode &codeOut, String &strOut, int &intOut);
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
};

#endif