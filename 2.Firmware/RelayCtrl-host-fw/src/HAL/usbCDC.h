#ifndef __USB_CDC_H
#define __USB_CDC_H
#include <stdint.h>

void usb_init();

int usb_send_data(uint8_t *buf, uint16_t len);
int usb_read_data(uint8_t *buf, uint16_t len);
int usb_avaulable();
void usb_cache_clear();

#endif