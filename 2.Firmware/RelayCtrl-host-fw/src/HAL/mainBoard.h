#ifndef __MAIN_BOARD_H
#define __MAIN_BOARD_H

#include "HAL_Def.h"

namespace HAL
{
    enum
    {
        STOP_ZERO = 0,
        STOP,
        GOTO_ZERO,
        EMPTY,
        BASIC,
        DROP,
        POINT_MOVE,
    };
    typedef uint8_t BoardRunState_t;

    enum
    {
        Little_Mask = 0,
        Big_Mask,
        Eye_Mask,
        Neck_Mask,
        T_Mask,
        Belly_Mask,
    };
    typedef uint8_t MaskType_t;

    typedef struct
    {
        uint8_t run_state;     // running mode: 0 stop zero, 1 stop not zero, 2 go to zero, 3 empot, 4 basic, 5 drop
        uint8_t progress;      // run schedule 0~100
        uint8_t uv_light;      // "uv" light switch state (0 close, 1 open)
        uint8_t water_pump;    // water pump state(0 close, 1 open)
        uint8_t stop_state;    // stop switch state (0 close, 1 open)
        uint8_t x_zero;        // "x" make zero switch (0 not zero, 1 zero)
        uint8_t y_zero;        // "y" make zero switch (0 not zero, 1 zero)
        uint8_t z_zero;        // "z" make zero switch (0 not zero, 1 zero)
        uint8_t water_too_low; // basic liquid too little(0 not low, 1 low)
        uint8_t error;         // run error
    } DeviceState_t;

    typedef struct
    {
        bool read;
        uint8_t x;
        uint8_t y;
        uint8_t z;
    } MoveOffset_t;
}

namespace HAL
{
    class MainBoard
    {
    private:
        uint8_t txbuff[256]; // 一包按240字节数据发送，最大使用248字节空间
        uint8_t rxbuff[256];

        FrameCheck_t frameCheck;
        DeviceState_t deviceState;

        MoveOffset_t offset;

    public:
        void
        init();

        void sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len);
        int sendData(uint8_t *buf, uint16_t len);
        int readData(uint8_t *buf, uint16_t len, uint16_t timeOut = 2000);
        bool checkReceiveData(uint8_t *buf, uint16_t len);
        bool checkCmdACK(const char *ack = "OK");
        void data_execute(uint8_t cmd, uint8_t *data, uint16_t len);

        void dataPack_handle();

        void saveDeviceState(uint8_t *data, uint16_t len);
        DeviceState_t *getDeviceState();
        uint8_t getDeviceRunState();

        void reqDeviceData(uint16_t time = 800);
        bool baseLiquid(bool onOff, uint8_t mask, uint8_t mil, uint8_t depth);
        bool dropLiquid(bool onOff, uint8_t mask, uint8_t mil, uint8_t depth);
        bool empty(bool onOff);
        void gotoZero();
        bool light(bool onOff);
        bool moveTestPoint(uint8_t point);
        bool setOffset(uint8_t select);
        bool moveTest(bool onOff);

        bool readOffset();
        String getOffsetMsg();
        bool writeOffset();

        bool upgrade(const char *filePath);

        void handle();
    };

} // namespace HAL

#endif