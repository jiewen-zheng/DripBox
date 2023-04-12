#ifndef __COMMAND_H
#define __COMMAND_H

#include <stdint.h>
#include <stdbool.h>

#define COMMAND_CRC_EN 1

typedef struct
{
    uint8_t cmd;
    uint8_t *data;
    uint16_t dataLen;
    uint16_t crc;
    uint8_t frame[128];
    uint16_t frameLen;
} CommType_t;

void comm_handle();
#endif