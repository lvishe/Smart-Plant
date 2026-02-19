#include "LClient.h"
#include "Sensors/ATH10_Sensor.h"
#include "Sensors/Moisture_Sensor.h"
#include "Sensors/Ph_Sensor.h"
#include "Sensors/Tds_Sensor.h"


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
    delay(loopDelay);
}