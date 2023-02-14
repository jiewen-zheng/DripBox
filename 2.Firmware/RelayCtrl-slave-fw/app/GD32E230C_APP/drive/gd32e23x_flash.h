#ifndef __GD32E23X_FLASH_H
#define __GD32E23X_FLASH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "gd32e23x.h"

#ifdef __cplusplus
}
#endif

#include <stdint.h>
#include <stdbool.h>

#define GD32E23X_FLASH_BASE_ADDR 0x8000000
#define GD32E23X_FLASH_PAGE_SIZE 0x400
#define GD32E23X_FLASH_PAGE_NUM 64

bool erase_flash_page(uint32_t startAddr, uint32_t endAddr);
bool program_flash(uint32_t startAddr, const uint32_t *wbuff, uint32_t len);
bool read_flash(uint32_t startAddr, uint32_t *rbuff, uint32_t len);

#endif
