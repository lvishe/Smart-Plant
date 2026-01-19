#include "../Tds_Sensor.h"
#include "../ATH10_Sensor.h"
#include "../Moisture_sensor.h" 
#include "../Ph_Sensor.h"

// 1. Define Sensors
AHT10Sensor    airTemp(AHTMode::TEMPERATURE);
AHT10Sensor    airHum(AHTMode::HUMIDITY);
TDSSensor      waterTDS(27, 3.3, 4096.0); // ESP32 Settings
PHSensor       waterPH(35, 3.3, 4096.0);  // ESP32 Settings

// 2. Create the list
Sensor* mySensors[] = {
    &airTemp,
    &airHum,
    &waterTDS,
    &waterPH
};

const int sensorCount = sizeof(mySensors) / sizeof(mySensors[0]);

void setup() {
    Serial.begin(115200);
    
    // Link TDS to Temp for compensation
    waterTDS.setTemperatureSource(&airTemp);

    // Optional: Calibrate pH if you know your probe's specific offset
    // waterPH.calibrate(3.48, 0.175); 

    Serial.println("Multisensor System Online");
}

void loop() {
    Serial.println("\n--- Measurements ---");

    for (int i = 0; i < sensorCount; i++) {
        float val = mySensors[i]->read_value();
        
        Serial.print(mySensors[i]->getName());
        Serial.print(": ");
        
        if (val < -900) {
            Serial.println("Check Connection!");
        } else {
            Serial.println(val, 2); // Print with 2 decimal places
        }
    }

    delay(3000);
}