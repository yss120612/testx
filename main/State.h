#ifndef __STATE_h
#define __STATE_h
#define VERSION 1
#define RELAYS_COUNT 2

struct __attribute__((__packed__)) SystemState_t
{
    uint8_t version : (uint8_t)VERSION;
    relState_t relays[RELAYS_COUNT];
    uint8_t crc;
};

class State{
    public:
    SystemState_t st;
};

#endif