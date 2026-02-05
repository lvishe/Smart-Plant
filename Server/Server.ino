#include "LServer.h"

Lserver server;

void setup() {
    Serial.begin(115200);
    server.startAP();
}

 int nextDelay = 1000;
 
void loop() {
    // Check for new client
    if (server.acceptClient()) {
        // New connection handled
    }

    Opcode code;
    String data;
   
    // 1. Check for incoming packet
    if (server.receivePacket(code, data)) {
        Serial.println("Received DATA: " + data);

        nextDelay++;

        // 3. Send the UPDATE packet
        if (server.sendUpdate(nextDelay)) {
            Serial.println("Sent Update: " + String(nextDelay));
        }
    }
}