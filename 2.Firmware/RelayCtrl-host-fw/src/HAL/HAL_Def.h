#ifndef __HAL_DEF_H
#define __HAL_DEF_H

#include <Arduino.h>
#include <HardwareSerial.h>

/* UART SCR */
#define SCR_SERIAL Serial1
#define SCR_UART_TX_PIN 17
#define SCR_UART_RX_PIN 18

/* UART MainBoard */
#define MB_SERIAL Serial2
#define MB_UART_TX_PIN 19
#define MB_UART_RX_PIN 20

typedef struct
{
    uint8_t *data;
    uint16_t dataLen;
    uint16_t crc;
    uint8_t *frame;
    uint16_t frameLen;
    uint8_t cmd;
    uint16_t buttonAddr;
    uint16_t buttonVal;
} FrameCheck_t;

#endif