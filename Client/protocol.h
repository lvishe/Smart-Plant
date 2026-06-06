#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum class MessageType : uint8_t 
{
    SetDelay = 0,
    SetThresholds = 1,
    Reboot = 2,
};

struct __attribute__((packed)) ServerMessage 
{
    MessageType type;    
    uint32_t data;
};

struct __attribute__((packed)) ClientMessage 
{
    uint32_t device_id;
    float    temp;
    float    humidity;
    uint16_t moisture;
    float    ph_level;
    uint16_t tds_value;   
};

#endif