#ifndef __MAIN_BOARD_H
#define __MAIN_BOARD_H

#include "HAL_Def.h"

namespace HAL
{
    class MainBoard
    {
    private:
        uint8_t txbuff[256]; // 一包按240字节数据发送，最大使用248字节空间
        uint8_t rxbuff[256];

        bool updateFlag;

    public:
        MainBoard() { updateFlag = false; }

        void
        init();

        bool sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len, bool checkFlag = true, int retry = 3);

        int sendData(uint8_t *buf, uint16_t len);
        int readData(uint8_t *buf, uint16_t len, uint16_t timeOut = 3000);

        bool upgrade();

        void setUpgrade(bool up) { updateFlag = up; }

        void handle();
    };

} // namespace HAL

#endif