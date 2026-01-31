#include "Moisture_Sensor.h"

MoistureSensor::MoistureSensor(int pin, int airValue = 1023, int waterValue = 0) 
    : Sensor("SoilMoist", pin), _airValue(airValue), _waterValue(waterValue)
{
    pinMode(_pin, INPUT);
}

float MoistureSensor::read_value() override 
{
    int rawMedian = analogRead(_pin); 
    return rawMedian;
}