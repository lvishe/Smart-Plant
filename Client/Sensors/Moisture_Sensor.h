#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

#include "Sensor.h"

class MoistureSensor : public Sensor 
{
    private:
        int _airValue;   // Calibration value when sensor is dry
        int _waterValue; // Calibration value when sensor is in water

    public:
        MoistureSensor(int pin, int airValue = 1023, int waterValue = 0);
        float read_value() override;
};

#endif