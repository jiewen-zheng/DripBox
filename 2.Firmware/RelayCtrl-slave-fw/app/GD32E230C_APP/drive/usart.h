#ifndef __USART_H
#define __USART_H
#include "fifo.h"

#include "gd32e23x.h"
#include <stdio.h>

#define USART0_FIFO_NUM 0
#define UART_TXBUFF_SIZE 64
#define UART_RXBUFF_SIZE 256

void usart0_init(uint32_t baud);
void usart0_send_data(uint8_t *buf, uint16_t len);
void usart0_dma_send_data(uint8_t *buf, uint16_t len);

void usart0_idle_handle();

extern uint8_t usart0_txbuf[UART_TXBUFF_SIZE];
volatile extern uint8_t usart0_rxbuf[UART_RXBUFF_SIZE];

#endif
