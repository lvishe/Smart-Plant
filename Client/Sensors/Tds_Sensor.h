
#include "Sensor.h"
#include <Wire.h>


class TDSSensor : public Sensor 
{
    private:
        static const int SCOUNT = 30;
        float _vref, _adcRange;
        Sensor* _tempSource = nullptr;

        int _getMedianSample()
        {
            int bTab[SCOUNT];
            for (int i = 0; i < SCOUNT; i++)
            {
                bTab[i] = analogRead(_pin);
                delay(1);
            }
            // Simple Sort
            for (int j = 0; j < SCOUNT - 1; j++)
            {
                for (int i = 0; i < SCOUNT - j - 1; i++)
                {
                    if (bTab[i] > bTab[i+1])
                    {
                        int t = bTab[i];
                        bTab[i] = bTab[i+1];
                        bTab[i+1] = t;
                    }
                }
            }
            return bTab[SCOUNT / 2];
        }

    public:
        TDSSensor(int pin, float vref = 3.3, float adcRange = 4096.0) 
            : Sensor("WaterTDS", pin), _vref(vref), _adcRange(adcRange)
        {
            pinMode(_pin, INPUT);
        }

        void setTemperatureSource(Sensor* source) { _tempSource = source; }

        float read_value() override 
        {
            float temp = (_tempSource) ? _tempSource->read_value() : 25.0;
            if (temp < -50) temp = 25.0; // Fallback if linked sensor fails

            float v = (float)_getMedianSample() * _vref / _adcRange;
            float ecComp = v / (1.0 + 0.02 * (temp - 25.0));
            return (133.42*pow(ecComp,3) - 255.86*pow(ecComp,2) + 857.39*ecComp) * 0.5;
        }
};