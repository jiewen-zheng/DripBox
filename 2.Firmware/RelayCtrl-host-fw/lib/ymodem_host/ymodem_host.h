#ifndef __YMODEM_HOST_H_
#define __YMODEM_HOST_H_

#include <Arduino.h>
#include "FS.h"

/* enable original ymodem */
// #define YH_ORIGINAL_EN

/* enable log */
#define YH_ERROR_LOG_EN

/* ymodem wait data max time (ms). */
#define YH_WAIT_TIME 3000

/* yomodem host, package send faile retry number */
#define YH_RETRY_NUM 5

/**
 * @brief ymodem callback type
 */
typedef int (*ymodem_cb_t)(uint8_t *pbuf, uint16_t len, uint16_t timeOut);

typedef struct
{
    ymodem_cb_t sendData_callback;
    ymodem_cb_t readData_callback;
} YmodemConfig_t;

class YmodemHost
{
private:
    YmodemConfig_t *ym_config;

    uint8_t *readBuff;
    uint8_t *sendBuff;

    bool _dynamic = false;
    bool _cancel;

public:
    YmodemHost(YmodemConfig_t *config);
    ~YmodemHost();

    /* If you pass arguments using constructs, you don't need to use "begin()" */
    bool begin(YmodemConfig_t *config, uint8_t *ptx, uint8_t *prx);
    void end();

    /* send file to slave*/
    bool sendFile(fs::FS &fs, const char *filePath);

    /* receive slave log info */
    void get_log();

private:
    /* ymodem start handle */
    bool start(const char *fileName, size_t size, int trynu = YH_RETRY_NUM);
    /* ymodem end handle */
    bool eot();

    /* send ymodem protocol package */
    bool sendPackage(uint8_t pindex, uint8_t *buf, uint16_t len, int retry = YH_RETRY_NUM);

    /* check slave ack data */
    int checkACK(uint8_t ack);

    /* clear all buffer data */
    void clear_cache();

    int sendByte(uint8_t byte);
    int sendData(uint8_t *pbuf, uint16_t len);
    int readData(uint8_t *pbuf, uint16_t len, uint16_t timeout = YH_WAIT_TIME);
};

#endif
