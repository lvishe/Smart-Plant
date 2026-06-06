#pragma once
#include "Sensor.h"

class MoistureSensor : public Sensor 
{
    private:
        // Adjusted defaults for ESP32 12-bit ADC (0-4095 range)
        int _airValue;   // Calibration value when sensor is totally dry 
        int _waterValue; // Calibration value when sensor is submerged in water

    public:
        // Set default air to 4095, and a typical water value to 1500 (adjust as needed)
        MoistureSensor(int pin, int airValue = 4095, int waterValue = 1500)
            : Sensor("SoilMoist", pin), _airValue(airValue), _waterValue(waterValue)
        {
            pinMode(_pin, INPUT);
        }

        float read_value() override
        {
            int rawADC = analogRead(_pin); 
            
            // Map the raw value to a percentage (0% to 100%)
            // Dry air (higher voltage) = 0%, Water (lower voltage) = 100%
            float moisturePercent = map(rawADC, _airValue, _waterValue, 0, 100);

            // Clamp values between 0% and 100% to prevent weird outputs if ADC drifts
            if (moisturePercent < 0.0f) moisturePercent = 0.0f;
            if (moisturePercent > 100.0f) moisturePercent = 100.0f;

            return moisturePercent;
        }
};