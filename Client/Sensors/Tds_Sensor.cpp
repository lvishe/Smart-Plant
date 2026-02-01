
#include "Tds_Sensor.h"
#include <math.h>

TDSSensor::TDSSensor(int pin, float vref, float adcRange) 
            : Sensor("WaterTDS", pin), _vref(vref), _adcRange(adcRange)
        {
            pinMode(_pin, INPUT);
        }

        void TDSSensor::setTemperatureSource(Sensor* source) { _tempSource = source; }

        float TDSSensor::read_value() 
        {
            float temp = (_tempSource) ? _tempSource->read_value() : 25.0;
            if (temp < -50) temp = 25.0; // Fallback if linked sensor fails

            float v = analogRead(_pin) * _vref / _adcRange;
            float ecComp = v / (1.0 + 0.02 * (temp - 25.0));
            
            return (133.42*pow(ecComp,3) - 255.86*pow(ecComp,2) + 857.39*ecComp) * 0.5;
        }