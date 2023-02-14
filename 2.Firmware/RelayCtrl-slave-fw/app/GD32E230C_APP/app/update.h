#ifndef __UPDATE_H
#define __UPDATE_H

#include <stdint.h>
#include "gd32e23x_flash.h"

/* update flag save address */
#define USER_UPDATE_FLAG_ADDR (GD32E23X_FLASH_BASE_ADDR + GD32E23X_FLASH_PAGE_SIZE * 63)
#define USER_UPDATE_FLAG 0x55AAA55A

/* app flash start address */
#define USER_APP_START_ADDR (GD32E23X_FLASH_BASE_ADDR + GD32E23X_FLASH_PAGE_SIZE * 15)

/* app max size */
#define USER_APP_MAX_SIZE (45 * GD32E23X_FLASH_PAGE_SIZE)

/* file name max length */
#ifndef YS_FILENAME_MAX_LEN
#define YS_FILENAME_MAX_LEN 32

typedef struct
{
  char Name[YS_FILENAME_MAX_LEN];
  uint32_t Size;
} FileInfo_t;
#endif

typedef struct
{
  uint32_t update_flag;
  FileInfo_t info;
} Update_t;

typedef union
{
  Update_t info;
  uint32_t data[YS_FILENAME_MAX_LEN + 8];
} UpdateUnion_t;

typedef void (*jumpToApp)();

void iap_load_app(uint32_t app_addr);

bool update_check();
void update_eraseFlag(FileInfo_t *info);
void update_setFlag();

extern const char *version;

#endif