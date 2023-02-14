#include "update.h"
#include "systick.h"
#include <string.h>

const char *version = "app_v2.0";

jumpToApp jump_to_app;

void iap_load_app(uint32_t app_addr)
{
    /* check stack address */
    if (((*(__IO uint32_t *)app_addr) & 0x2FFE0000) == 0x20000000)
    {
        jump_to_app = (jumpToApp) * (__IO uint32_t *)(app_addr + 4);

        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t *)app_addr);
        jump_to_app();
    }
    else
    {
        NVIC_SystemReset();
    }
}

/**
 * @brief get firmware infomation
 */
Update_t update_getMessage()
{
    Update_t up;

    read_flash(USER_UPDATE_FLAG_ADDR, (uint32_t *)&up, sizeof(Update_t));

    return up;
}

/**
 * @brief write firmware infomation to flash
 */
void update_writeMessage(Update_t *info)
{
    UpdateUnion_t up_union;
    up_union.info = *info;

    erase_flash_page(USER_UPDATE_FLAG_ADDR, USER_UPDATE_FLAG_ADDR + sizeof(Update_t) - 1);
    program_flash(USER_UPDATE_FLAG_ADDR, (uint32_t *)&up_union, sizeof(UpdateUnion_t));
}

/**
 * @brief check update flag
 */
bool update_check()
{
    uint64_t app_start;
    Update_t info = update_getMessage();

    read_flash(USER_APP_START_ADDR, (uint32_t *)&app_start, 8);

    if (info.update_flag == USER_UPDATE_FLAG || app_start == 0xFFFFFFFFFFFFFFFF)
    {
        return true;
    }
    return false;
}

/**
 * @brief clear update flag
 */
void update_eraseFlag(FileInfo_t *info)
{
    if (info == NULL)
        return;

    Update_t update;

    update.info = *info;
    update.update_flag = 0xffffffff;

    update_writeMessage(&update);
}

/**
 * @brief set update flag
 */
void update_setFlag()
{
    Update_t update;
    update.update_flag = USER_UPDATE_FLAG;

    update_writeMessage(&update);
}