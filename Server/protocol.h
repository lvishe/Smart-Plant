#ifndef PROTOCOL_H
#define PROTOCOL_H

enum class Opcode : int
{
    DATA = 0, 
    UPDATE
};

struct Message 
{
    Opcode code;
    int data_lenght;
};

#endif