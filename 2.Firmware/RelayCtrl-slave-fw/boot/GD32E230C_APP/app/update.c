#include "update.h"
#include "systick.h"
#include "fifo.h"
#include "usart.h"
#include <string.h>

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

/* ymodem slave callback function realize */
int read_data_cb(void *msg)
{
    uint32_t wait_time = HAL_GetTick();

    Yslave_transfer_t *trans = (Yslave_transfer_t *)msg;

    uint16_t olen = 0;
    do
    {
        olen = fifo_dynamic_get_occupy_size(USART0_FIFO_NUM);
        delay_ms(1);
    } while ((olen < trans->len) && (HAL_GetTick() - wait_time < trans->timeout));

    return fifo_dynamic_read(USART0_FIFO_NUM, trans->pbuf, trans->len);
}

int send_data_cb(void *msg)
{
    Yslave_transfer_t *trans = (Yslave_transfer_t *)msg;

    usart0_send_data(trans->pbuf, trans->len);

    return -1;
}

int clear_cb(void *msg)
{
    fifo_dynamic_clear(USART0_FIFO_NUM);
    memset((uint8_t *)usart0_rxbuf, 0, sizeof(usart0_rxbuf));

    return -1;
}

int store_cb(void *msg)
{
    Yslave_store_t *store = (Yslave_store_t *)msg;

    bool s = program_flash(store->store_addr, store->store_data, store->store_size);

    if (!s)
    {
        return ys_storeError;
    }

    return -1;
}

void update_init()
{
    static Yslave_config_t config = {
        .storeStartAddr = USER_APP_START_ADDR,
        .maxFilesize = USER_APP_MAX_SIZE,
        .store_callback = store_cb,
        .sendData_callback = send_data_cb,
        .readData_callback = read_data_cb,
    };

    erase_flash_page(USER_APP_START_ADDR, USER_APP_START_ADDR + USER_APP_MAX_SIZE - 1);

    if (!yslave_init(&config))
    {
        printf("ymodem slave init failed\r\n");
    }
}

void update_app()
{
    Yslave_status_t s = yslave_handle();

    if (s == ys_succeed)
    {
        update_eraseFlag(yslave_getFileInfo());
        delay_ms(100);
        NVIC_SystemReset();
    }
    else if (s == ys_storeError)
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