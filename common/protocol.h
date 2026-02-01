#ifndef PROTOCOL_H
#define PROTOCOL_H

enum class Opcode : int
{
    DATA = 0,   // Client -> Server: Sensor data string
    UPDATE = 1, // Server -> Client: New delay interval (int)
};

// Fixed size header sent before every variable length payload
struct PacketHeader
{
    Opcode code;
    int data_length;
};

#endif