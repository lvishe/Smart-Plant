#include <Arduino.h>
#include <Wire.h> // Required for I2C (AHT10)
#include "LClient.h"
#include "protocol.h"

// Sensor Includes
#include "Sensors/ATH10_Sensor.h"
#include "Sensors/Moisture_Sensor.h"
#include "Sensors/Ph_Sensor.h"
#include "Sensors/Tds_Sensor.h"

// Networking & Control Variables
Lclient myClient;
uint32_t loopDelay = 3000; // Default wait time
uint32_t myDeviceId = 0;   // Dynamically assigned ID

// Initialize Sensor Objects
AHT10Sensor    airTemp(AHTMode::TEMPERATURE);
AHT10Sensor    airHum(AHTMode::HUMIDITY);
LPHSensor      ph(1);
TDSSensor      tds(0);
MoistureSensor m(4);

void setup() 
{
    Serial.begin(115200);
    delay(1000);
    
    // 1. Initialize I2C and Sensors
    Wire.begin(21, 22);
    airTemp.init();
    airHum.init();
    Serial.println("Sensors Initialized");

    // 2. Generate a unique ID for this client using its hardware MAC address
    uint64_t mac = ESP.getEfuseMac();
    myDeviceId = (uint32_t)(mac & 0xFFFFFF); // Keep the lower 24 bits

    // 3. Connect to Server
    Serial.println("Connecting to Server...");
    myClient.begin();
    Serial.println("Connected!");
}

void loop() 
{
    // 1. Pack the struct with REAL sensor data
    ClientMessage myData;
    myData.device_id = myDeviceId; 
    
    myData.temp      = airTemp.read_value();
    myData.humidity  = airHum.read_value();
    myData.moisture  = m.read_value();
    myData.ph_level  = ph.read_value();
    myData.tds_value = tds.read_value();

    // Debug print to Serial Monitor
    Serial.printf("\n--- Reading Data ---\n");
    Serial.printf("Temp: %.2f, Hum: %.2f, Moist: %.2f, pH: %.2f, TDS: %.2f\n", 
                  myData.temp, myData.humidity, myData.moisture, myData.ph_level, myData.tds_value);

    // 2. Send the binary struct
    Serial.println("Sending sensor data...");
    if (myClient.sendPacket(myData))
    {
        Serial.println("Send OK!");
    }
    else
    {
        Serial.println("Send FAILED!");
    }

    // 3. Check for a configuration reply from the server
    ServerMessage config;
    delay(100); // Wait a moment for network travel
    
    if (myClient.receivePacket(config)) 
    {
        if (config.type == MessageType::SetDelay) 
        {
            loopDelay = config.data;
            Serial.printf("New interval set: %d ms\n", loopDelay);
        }
    }

    Serial.println("-----------------------------------");
    delay(loopDelay);
}