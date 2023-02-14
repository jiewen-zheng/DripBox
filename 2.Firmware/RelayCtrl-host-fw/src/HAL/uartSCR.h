#ifndef __UART_SCR_H
#define __UART_SCR_H

#include "HAL_Def.h"
#include "FS.h"

#define FRAME_HEAD 0x5AA5
#define SAVE_RAM_ADDR 0x8000

enum
{
    CHECK_NONE = 0,
    CHECK_ACK,
    CHECK_DATA
};
typedef uint8_t CheckType_t;

enum
{
    button_update = 0x1000,
};
typedef uint16_t ButtonAddr_t;

namespace HAL
{
    class UART_SCR
    {
    public:
        uint8_t txbuff[256]; // 一包按240字节数据发送，最大使用248字节空间
        uint8_t rxbuff[256];

    public:
        void init();
        void reset();

        bool readDataFormRam(uint16_t regAddr, int readLen);
        bool readDataFormFlash(uint16_t flashAddr, int readLen);

        void handle();
        void button_handle(uint16_t addr, uint16_t val);
        void upgrade();

    private:
        uint16_t sendData(uint8_t *buf, uint16_t len);
        bool sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len, bool checkFlag = true, int retry = 3);
        uint16_t readData(uint8_t *buf, uint16_t len, uint16_t timeOut = 2000);

        bool writeFileToRam(File &file, uint16_t fileSize, uint16_t regAddr); // 一次最大写入32kb
        bool saveToFlash(uint16_t flashAddr, uint16_t ramAddr);
        bool writeFile(const char *filePath, uint8_t saveID);

        bool checkReceiveFrame(uint8_t *buf, uint16_t len);
        bool check(CheckType_t type, uint16_t rlen = 8); // ack 校验默认响应长度为8

    private:
        typedef struct 
        {
            uint8_t *data;
            uint8_t dataLen;
            uint8_t *frame;
            uint8_t frameLen;
            uint8_t cmd;
            uint16_t buttonAddr;
            uint16_t buttonVal;
            uint16_t crc;
        } FrameCheck_t;
        FrameCheck_t frameCheck;
    };
}

#endif