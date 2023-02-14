#ifndef _EF_DISKIO_H_
#define _EF_DISKIO_H_

#include <Arduino.h>
#include <SPI.h>
#include "w25qx_define.h"

uint8_t exflash_uninit(uint8_t pdrv);
uint8_t exflash_init(uint8_t cs, SPIClass *spi, int hz);

uint8_t efReadStatus(uint8_t pdrv);
void efReadData(uint8_t pdrv, uint32_t addr, void *buf, uint16_t len);
void efWriteData(uint8_t pdrv, uint32_t addr, const void *buf, uint16_t len);

uint8_t exflash_unmount(uint8_t pdrv);
bool exflash_mount(uint8_t pdrv, const char *path, uint8_t max_files, bool format_if_empty);
void exflash_clear(uint8_t pdrv);
#endif