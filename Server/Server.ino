#include "LServer.h"

Lserver server;
int nextDelay = 5000;

void setup() 
{
    Serial.begin(115200);
    server.startAP();
    Serial.println("Access Point Started");
}

void loop() 
{
    // Check for new client connections
    if (server.acceptClient())
    {
        Serial.println("New client connected!");
    }

    Opcode code;
    String data;
    String remoteIP; // Variable to store the sender's IP

    // 1. Check for incoming packet and capture the sender's IP
    // Note: Assuming your Lserver class has a way to provide the IP
    if (server.receivePacket(code, data, remoteIP)) 
    {
        Serial.print("Received from IP: ");
        Serial.println(remoteIP);
        Serial.println("Data content: " + data);


        nextDelay++;

        // 3. Send the UPDATE packet back to that specific client
        // Ensure sendUpdate is targeting the active client connection
        if (server.sendUpdate(nextDelay)) 
        {
            Serial.print("Sent Update to ");
            Serial.print(remoteIP);
            Serial.print(": ");
            Serial.println(nextDelay);
        }
    }
}