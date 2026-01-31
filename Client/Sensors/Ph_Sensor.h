#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "Sensor.h"

class PHSensor : public Sensor
{
    private:
        float _vref;
        float _adcRange;
        
        // Calibration values
        float _neutralVoltage = 3.5;  // Voltage at pH 7.0
        float _slope = 0.18;         // Voltage change per pH unit

    public:
        
        PHSensor(int pin, float vref = 3.3, float adcRange = 4096.0);

        // Allows for manual calibration
        void calibrate(float neutralVolts, float slopeValue);
        
        float read_value() override;
};

#endif