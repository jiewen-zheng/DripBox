#ifndef __SCR_CRC16_H
#define __SCR_CRC16_H
#include "stdint.h"

uint16_t calc_crc16(uint8_t *buf, uint16_t len);

#endif