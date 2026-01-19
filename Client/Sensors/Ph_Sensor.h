#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "Sensor.h"

class PHSensor : public Sensor {
    private:
        static const int SCOUNT = 30; 
        float _vref;
        float _adcRange;
        
        // Calibration values
        float _neutralVoltage = 3.5;  // Voltage at pH 7.0
        float _slope = 0.18;         // Voltage change per pH unit

        int _getMedianSample() {
            int bTab[SCOUNT];
            for (int i = 0; i < SCOUNT; i++) {
                bTab[i] = analogRead(_pin);
                delay(2); 
            }
            // Sort logic (Bubble Sort)
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
         * @param pin Analog pin
         * @param vref Voltage reference (5.0 for Uno, 3.3 for ESP32)
         * @param adcRange ADC resolution (1024.0 for Uno, 4096.0 for ESP32)
         */
        PHSensor(int pin, float vref = 5.0, float adcRange = 1024.0) 
            : Sensor("PH_Meter", pin), _vref(vref), _adcRange(adcRange) {
            pinMode(_pin, INPUT);
        }

        // Allows for manual calibration
        void calibrate(float neutralVolts, float slopeValue) {
            _neutralVoltage = neutralVolts;
            _slope = slopeValue;
        }

        float read_value() override {
            // 1. Get stable raw value
            int rawADC = _getMedianSample();

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

#endif