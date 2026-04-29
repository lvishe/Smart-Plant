#ifndef _SENSOR_H_
#define _SENSOR_H_

class Sensor 
{
    protected:
        const char* _name;
        const int _pin;

    public:  
        Sensor(const char* name, int pin) : _name(name), _pin(pin) {}
        virtual ~Sensor() {}

    public:
        virtual float read_value() = 0;
        const char* getName() const { return _name; }
};

#endif