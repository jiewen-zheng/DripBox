#ifndef __YMODEM_SLAVE_H
#define __YMODEM_SLAVE_H

/**
 * @attention
 * @author monster
 */

#include <stdint.h>
#include <stdbool.h>

/* enable original ymodem */
// #define YS_ORIGINAL_EN

/* enable log */
// #define YS_LOG_EN

/* ymodem wait data max time(ms), Modified according to mcu frequency */
#define YS_WAIT_TIME 3000

/* ymodem transfer fail retry number */
#define YS_RETRY_NUM 5

/* file name max length */
#define YS_FILENAME_MAX_LEN 32

enum
{
    ys_succeed = 0,
    ys_unknow,
    ys_timeout,
    ys_transferError,
    ys_fileToolong,
    ys_cancle,
    ys_storeError,
};
typedef uint8_t Yslave_status_t;

/**
 * @brief ymodem slave callback type
 */
typedef int (*yslave_cb_t)(void *msg);

/**
 * @brief ymodem slave transfer data type
 */
typedef struct
{
    uint8_t *pbuf;
    uint32_t len;
    uint32_t timeout;
} Yslave_transfer_t;

/**
 * @brief ymodem slave store data type
 */
typedef struct
{
    uint32_t store_addr;
    uint32_t *store_data;
    uint32_t store_size;
} Yslave_store_t;

typedef struct ymodem_slave
{
    uint32_t storeStartAddr; // flash file store start address
    uint32_t maxFilesize;    // file max size
    yslave_cb_t store_callback;

    yslave_cb_t sendData_callback;
    yslave_cb_t readData_callback;
} Yslave_config_t;

typedef struct
{
    char Name[YS_FILENAME_MAX_LEN];
    uint32_t Size;
} FileInfo_t;

bool yslave_init(Yslave_config_t *config);
Yslave_status_t yslave_handle();

FileInfo_t *yslave_getFileInfo();

#endif