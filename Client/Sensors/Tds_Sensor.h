#ifndef _SENSOR_TDS_H_
#define _SENSOR_TDS_H_

#include "Sensor.h"
#include <math.h>

class TDSSensor : public Sensor 
{
    private:
        static const int SCOUNT = 3;
        float _vref, _adcRange;
        Sensor* _tempSource = nullptr;

        
    public:
        TDSSensor(int pin, float vref = 3.3, float adcRange = 4096.0)
            : Sensor("WaterTDS", pin), _vref(vref), _adcRange(adcRange)
        {
            pinMode(_pin, INPUT);
        }

        void setTemperatureSource(Sensor* source)
        { 
            _tempSource = source;
        }



        float read_value() override
        {
            float temp = (_tempSource) ? _tempSource->read_value() : 25.0;
            
            if (temp < -50) 
            {
                temp = 25.0;
            }

            float v = analogRead(_pin) * _vref / _adcRange;
            float ecComp = v / (1.0 + 0.02 * (temp - 25.0));
            
            return (133.42*pow(ecComp,3) - 255.86*pow(ecComp,2) + 857.39*ecComp) * 0.5;
        }
};


#endif