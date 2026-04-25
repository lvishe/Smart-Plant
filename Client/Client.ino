#include <Arduino.h>
#include "LClient.h"
#include "protocol.h"

Lclient myClient;
uint32_t loopDelay = 3000; // Default wait time

int num = 20;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Connecting to Server...");
    myClient.startSTA();
    Serial.println("Connected!");
}

void loop() {
    // 1. Pack the struct with mock sensor data
    
    ClientMessage myData;
    myData.device_id = 0xABC123; 
    myData.temp = num; //random(200, 300) / 10.0;    // Random temp 20.0 - 30.0
    myData.humidity = num - 2;//random(400, 600) / 10.0; // Random humidity 40.0 - 60.0
    myData.moisture = num - 4;//random(300, 800);
    myData.ph_level = num - 6;//6.5f;
    myData.tds_value = num -8;//450;

    num = num + 1;

    // 2. Send the binary struct
    Serial.println("\nSending sensor data...");
    if (myClient.sendPacket(myData)) {
        Serial.println("Send OK!");
    } else {
        Serial.println("Send Failed.");
    }

    // 3. Check if the server replied with an update
    ServerMessage incomingUpdate;
    // Give the server a few milliseconds to process and reply
    delay(50); 
    
    if (myClient.receivePacket(incomingUpdate)) {
        if (incomingUpdate.type == MessageType::SetDelay) {
            loopDelay = incomingUpdate.delay_ms;
            Serial.printf("Server commanded new delay: %d ms\n", loopDelay);
        }
    }

    // 4. Wait before sending the next reading
    delay(loopDelay);
}