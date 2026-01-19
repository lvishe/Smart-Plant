#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

#include "Sensor.h"

class MoistureSensor : public Sensor {
    private:
        static const int SCOUNT = 15; // Smaller buffer for faster response
        int _airValue;   // Calibration value when sensor is dry
        int _waterValue; // Calibration value when sensor is in water

        // Reusing our Median Filter logic for stability
        int _getMedianSample() {
            int bTab[SCOUNT];
            for (int i = 0; i < SCOUNT; i++) {
                bTab[i] = analogRead(_pin);
                delay(1);
            }
            // Sort
            for (int j = 0; j < SCOUNT - 1; j++) {
                for (int i = 0; i < SCOUNT - j - 1; i++) {
                    if (bTab[i] > bTab[i+1]) {
                        int t = bTab[i]; bTab[i] = bTab[i+1]; bTab[i+1] = t;
                    }
                }
            }
            return bTab[SCOUNT / 2];
        }

    public:
        /**
         * @param pin The analog pin
         * @param airValue The raw ADC reading when dry (e.g., 3200 for ESP32)
         * @param waterValue The raw ADC reading when in water (e.g., 1200 for ESP32)
         */
        MoistureSensor(int pin, int airValue = 1023, int waterValue = 0) 
            : Sensor("SoilMoist", pin), _airValue(airValue), _waterValue(waterValue) {
            pinMode(_pin, INPUT);
        }

        float read_value() override {
            int rawMedian = _getMedianSample();

            // Map the raw value to a 0-100% range
            // Note: airValue is usually HIGHER than waterValue on capacitive sensors
            float percentage = map(rawMedian, _airValue, _waterValue, 0, 100);

            // Ensure the value stays within 0-100%
            if (percentage < 0) percentage = 0;
            if (percentage > 100) percentage = 100;

            return percentage;
        }
};

#endif