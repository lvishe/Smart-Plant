#pragma once

#include "Sensor.h"


class LPHSensor : public Sensor
{
    private:
        float _vref;
        float _adcRange;
        
        // Calibration values
        float _neutralVoltage = 3.5;  // Voltage at pH 7.0
        float _slope = 0.18;         // Voltage change per pH unit

    public:
        
        LPHSensor(int pin, float vref = 3.3, float adcRange = 4096.0)
            : Sensor("PH_Meter", pin), _vref(vref), _adcRange(adcRange)
        {
            pinMode(_pin, INPUT);
        }


        // Allows for manual calibration
        void calibrate(float neutralVolts, float slopeValue)
        {
            _neutralVoltage = neutralVolts;
            _slope = slopeValue;
        }

        float read_value() override
        {
            // 1. Get stable raw value
            float rawADC = analogRead(_pin);

            // 2. Convert to voltage
            float voltage = (float)rawADC * _vref / _adcRange;

            // 3. Calculate pH based on your formula:
            // PH_step = (NeutralVoltage - CurrentVoltage) / Slope
            // PH = 7.0 + PH_step
            float phStep = (_neutralVoltage - voltage) / _slope;
            float phValue = 7.0 + phStep;

            // 4. Constrain to valid pH scale
            if (phValue < 0) phValue = 0.0;
            if (phValue > 14) phValue = 14.0;

            return phValue;
        }
};

