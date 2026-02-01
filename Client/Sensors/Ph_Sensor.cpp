#include "Ph_Sensor.h"

PHSensor::PHSensor(int pin, float vref, float adcRange) 
    : Sensor("PH_Meter", pin), _vref(vref), _adcRange(adcRange)
{
    pinMode(_pin, INPUT);
}

// Allows for manual calibration
void PHSensor::calibrate(float neutralVolts, float slopeValue) 
{
    _neutralVoltage = neutralVolts;
    _slope = slopeValue;
}

float PHSensor::read_value()
{
    // 1. Get stable raw value
    float rawADC = analogRead(_pin);

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