#include "../Tds_Sensor.h"
#include "../ATH10_Sensor.h"
#include "../Moisture_sensor.h" 
#include "../Ph_Sensor.h"

#define SENSOR_PH_PIN_NUMBER    1
 
// 1. Define Sensors
AHT10Sensor    airTemp(AHTMode::TEMPERATURE);
AHT10Sensor    airHum(AHTMode::HUMIDITY);
PHSensor       ph(1);
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
    Wire.begin(21,22);
    
    airTemp.init();
    airHum.init();

    Serial.println("System Online");
}

void loop() {
    Serial.println("\n--- Measurements ---");

    for (int i = 0; i < sensorCount; i++)
     {
        float val = mySensors[i]->read_value();
        
        Serial.print(mySensors[i]->getName());
        Serial.print(": ");
        
        if (val < -900) {
            Serial.println("Check Connection!");
        } else {
            Serial.println(val, 2);
        }
    }
    
    time_t previous = 0;
    tmie_t current = milis();

    delay(3000);
}