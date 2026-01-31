#ifndef _SENSOR_TDS_H_
#define _SENSOR_TDS_H_

#include "Sensor.h"

class TDSSensor : public Sensor 
{
    private:
        static const int SCOUNT = 3;
        float _vref, _adcRange;
        Sensor* _tempSource = nullptr;


    public:
        TDSSensor(int pin, float vref = 3.3, float adcRange = 4096.0);

        void setTemperatureSource(Sensor* source);

        float read_value() override;
};


#endif