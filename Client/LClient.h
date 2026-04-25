#ifndef LCLIENT_H
#define LCLIENT_H

#include <WiFi.h>
#include "protocol.h"

class Lclient {
private:
    WiFiClient _client;

public:
    void startSTA();
    
    // Pass the struct directly
    bool sendPacket(ClientMessage &msg);
    
    // Read directly into the struct
    bool receivePacket(ServerMessage &msgOut);
};

#endif