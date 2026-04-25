#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>


enum class MessageType : uint8_t
{
    SetDelay = 0,
    SetThresholds = 1,
    Reboot = 2,
    CalibrateSensors = 3
};

/*
// Data for specific commands
struct __attribute__((packed)) ThresholdConfig
{
    float min_ph;
    float max_ph;
    uint16_t min_moisture;
};
*/

struct __attribute__((packed)) ServerMessage 
{
    MessageType type;    
    uint32_t delay_ms;
};


struct __attribute__((packed)) ClientMessage
{
    uint32_t device_id;   // 4 bytes: Unique ID for the ESP32
    
    // Sensor Data
    float    temp;        // 4 bytes
    float    humidity;    // 4 bytes
    uint16_t moisture;    // 2 bytes (0-1024 or 0-4095)
    float    ph_level;    // 4 bytes
    uint16_t tds_value;   // 2 bytes    
};

#endif