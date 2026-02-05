#include "LClient.h"

// Global objects and variables
Lclient comm;
int loopDelay = 2000; // Default delay (2 seconds)

void setup() {
    Serial.begin(115200);
    
    // Connect to the Server's Access Point
    Serial.println("Starting Client...");
    comm.startSTA(); 
}

void loop() {
    // 1. Prepare the sensor data string
    // In a real scenario, read your actual sensors here
    float temp = 23.5;
    float humidity = 50.2;
    float soilMoisture = 101.3; // Example value that triggers server logic
    
    String sensorMsg = String(temp, 1) + "," + String(humidity, 1) + "," + String(soilMoisture, 1);

    // 2. Send the data as ONE packet
    Serial.print("Sending sensor data: ");
    Serial.println(sensorMsg);
    
    if (comm.sendPacket(sensorMsg, Opcode::DATA)) {
        Serial.println("Packet sent successfully.");
    } else {
        Serial.println("Failed to send packet (Server disconnected?)");
    }

    // 3. Try to receive a response (UPDATE) from the server
    Opcode incomingOp;
    String incomingStr;
    int incomingInt = 0;

    // We check for a response. This is non-blocking if no data is there.
    if (comm.receivePacket(incomingOp, incomingStr, incomingInt)) {
        if (incomingOp == Opcode::UPDATE) {
            loopDelay = incomingInt; // Update our global delay
            Serial.print(">>> Server command: New loop delay set to ");
            Serial.print(loopDelay);
            Serial.println("ms");
        } else if (incomingOp == Opcode::DATA) {
            Serial.print(">>> Server message: ");
            Serial.println(incomingStr);
        }
    }

    // 4. Wait for the specified interval
    Serial.println("-----------------------------------");
    delay(loopDelay);
}