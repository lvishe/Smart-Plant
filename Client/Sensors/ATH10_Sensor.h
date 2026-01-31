#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "Sensor.h"

enum class AHTMode { TEMPERATURE, HUMIDITY };

class AHT10Sensor : public Sensor
{
    private:
       Adafruit_Sensor* _s = nullptr; 
        AHTMode _mode;  

        // STATIC: Shared driver for all instances of this class.
        // The AHT10 is one physical chip, so we need one driver to manage it.
        static Adafruit_AHTX0 _sharedAHT;
        static bool _isInitialized;
    
    public:
        AHT10Sensor(AHTMode mode) : 
            Sensor(mode == AHTMode::TEMPERATURE ? "AirTemp" : "AirHum", -1),
            _mode(mode) {}

        bool init() 
        {
            if (!_isInitialized)
            {
                if (!_sharedAHT.begin()) 
                {
                    return false;
                }

                _isInitialized = true;
            }

            if (_mode == AHTMode::TEMPERATURE) 
            {
                _s = _sharedAHT.getTemperatureSensor();
            } 
            else
            {
                _s = _sharedAHT.getHumiditySensor();
            }

            return true;
        }

        float read_value() override 
        {
            if (!_isInitialized || _s == nullptr) 
            {
                if (!init()) return -1.0;
            }

            sensors_event_t event;
            _s->getEvent(&event);

            if (_mode == AHTMode::TEMPERATURE)
            {
                return event.temperature;
            } 
            else
            {
                return event.relative_humidity; 
            }
        }
};

Adafruit_AHTX0 AHT10Sensor::_sharedAHT;
bool AHT10Sensor::_isInitialized = false;