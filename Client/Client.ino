<<<<<<< HEAD
#include <Arduino.h>
#include "LClient.h"
#include "protocol.h"

Lclient myClient;
uint32_t loopDelay = 3000; // Default wait time
=======
#include "LClient.h"
#include "Sensors/ATH10_Sensor.h"
#include "Sensors/Moisture_Sensor.h"
#include "Sensors/Ph_Sensor.h"
#include "Sensors/Tds_Sensor.h"
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35

int num = 20;

<<<<<<< HEAD
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
=======
// Global objects and variables
Lclient comm;
int loopDelay = 2000; 

AHT10Sensor    airTemp(AHTMode::TEMPERATURE);
AHT10Sensor    airHum(AHTMode::HUMIDITY);
LPHSensor       ph(1);
TDSSensor      tds(0);
MoistureSensor m(4);

Sensor* mySensors[] = 
{
    &airTemp,
    &airHum,
    &tds,
    &ph,
    &m,
};

const int sensorCount = sizeof(mySensors) / sizeof(mySensors[0]);

void setup() 
{
    Serial.begin(115200);
    Wire.begin(21, 22);
    
    //airTemp.init();
    //airHum.init();

    Serial.println("System Online");
    Serial.println("Starting Client...");
    comm.startSTA(); 
}

void loop() 
{
    String sensorMsg = "";

    for (int i = 0; i < sensorCount; i++)
    {
        float val = mySensors[i]->read_value();
        
        Serial.print(mySensors[i]->getName());
        Serial.print(": ");
        
        if (val < -900) 
        {
            Serial.println("Check Connection!");
            sensorMsg += "ERR";
        } 
        else 
        {
            Serial.println(val, 2);
            sensorMsg += String(val, 1);
        }

        // Add a comma between values for the server to parse
        if (i < sensorCount - 1)
        {
            sensorMsg += ",";
        }
    }

    Serial.print("Sending sensor data: ");
    Serial.println(sensorMsg);
    
    if (comm.sendPacket(sensorMsg, Opcode::DATA)) 
    {
        Serial.println("Packet sent successfully.");
    } 
    else 
    {
        Serial.println("Failed to send packet (Server disconnected?)");
        comm.startSTA(); 
    }

    // Handle incoming responses from server
    Opcode incomingOp;
    String incomingStr;
    int incomingInt = 0;

    if (comm.receivePacket(incomingOp, incomingStr, incomingInt)) 
    {
        if (incomingOp == Opcode::UPDATE) 
        {
            loopDelay = incomingInt; 
            Serial.print(">>> Server command: New loop delay set to ");
            Serial.print(loopDelay);
            Serial.println("ms");
        } 
        else if (incomingOp == Opcode::DATA) 
        {
            Serial.print(">>> Server message: ");
            Serial.println(incomingStr);
        }
    }

    Serial.println("-----------------------------------");
>>>>>>> 6aadd463b6d1fff5c24b0226862f0d128d1ccd35
    delay(loopDelay);
}