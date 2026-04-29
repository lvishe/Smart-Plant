#include <Arduino.h>
#include "LClient.h"
#include "protocol.h"

Lclient myClient;
uint32_t loopDelay = 3000; // Default wait time
uint32_t myDeviceId = 0;   // Dynamically assigned ID

int num = 40;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Generate a unique ID for this client using its hardware MAC address
    uint64_t mac = ESP.getEfuseMac();
    myDeviceId = (uint32_t)(mac & 0xFFFFFF); // Keep the lower 24 bits for a short unique integer

    Serial.println("Connecting to Server...");
    myClient.begin();
    Serial.println("Connected!");
}

void loop() {
    // 1. Pack the struct with mock sensor data
    ClientMessage myData;
    myData.device_id = myDeviceId; // Use our unique hardware ID
    
    myData.temp = num; 
    myData.humidity = num - 2;
    myData.moisture = num - 4;
    myData.ph_level = num - 6;
    myData.tds_value = num - 8;

    num = num + 1;

    // 2. Send the binary struct
    Serial.println("\nSending sensor data...");
    if (myClient.sendPacket(myData))
    {
        Serial.println("Send OK!");
    }

    // Check for a configuration reply from the server
    ServerMessage config;
    delay(100); // Wait a moment for network travel
    if (myClient.receivePacket(config)) 
    {
        if (config.type == MessageType::SetDelay) {
            loopDelay = config.data;
            Serial.printf("New interval set: %d ms\n", loopDelay);
        }
    }

    delay(loopDelay);
}