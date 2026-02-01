#include "Moisture_Sensor.h"

MoistureSensor::MoistureSensor(int pin, int airValue, int waterValue) 
    : Sensor("SoilMoist", pin), _airValue(airValue), _waterValue(waterValue)
{
    pinMode(_pin, INPUT);
}

float MoistureSensor::read_value() 
{
    int rawMedian = analogRead(_pin); 
    return rawMedian;
}