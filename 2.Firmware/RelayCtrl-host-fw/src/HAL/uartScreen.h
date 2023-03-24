#ifndef __UART_SCR_H
#define __UART_SCR_H

#include "HAL_Def.h"
#include "FS.h"

#include "mainBoard.h"

#define FRAME_HEAD 0x5AA5
#define SAVE_RAM_ADDR 0x8000

enum
{
    CHECK_NONE = 0,
    CHECK_ACK,
    CHECK_DATA
};
typedef uint8_t CheckType_t;

namespace HAL
{
    enum
    {
        btn_baseStatrt = 0x1000,        // 基础夜开始按键
        btn_dropStart = 0x1001,         // 滴液开始按键
        btn_depthSelect = 0x1002,       // 深浅选择按键
        btn_emptying = 0x1004,          // 排空按键
        btn_popup_empty_start = 0x1005, // 弹窗排空_开始
        btn_milliliter = 0x1007,        // 毫升选择按键
        btn_popup_empty_stop = 0x1006,  // 弹窗排空_停止
        btn_popup_milliliter = 0x1008,  // 弹窗毫升
        btn_maskSelect = 0x1009,        // 面膜左右选择，1左，2右
        btn_calibrat = 0x100B,          // 校准页面按钮

        btn_connectWiFi = 0x1020,          // 连接wifi
        btn_popup_connectFaile = 0x1027,   // 弹窗连接失败
        btn_popup_connectSucceed = 0x1028, // 弹窗连接成功

        btn_firmwareUpdate = 1024,
        btn_softwareUpdate = 1025,

        btn_testRadeWrite = 0x1037, // 偏移值读取，1读，2写
        btn_testStart = 0x1036,     // 开始便宜测试
        btn_testPoint = 0x1030,     // 移动到点位选择，1左眼，2右眼，3嘴巴
        btn_testMove = 0x1034,      // 偏移选择，1上，2下，3左，4右
    };
    typedef uint16_t ScreenButtonAddr_t;

    enum
    {
        icon_baseStart = 0x1010,   // 基础液开始图标地址
        icon_dropStart = 0x1011,   // 深浅图标
        icon_depthSelect = 0x1012, // 深浅选择图标，0深，1浅
        icon_emptying = 0x1014,    // 排空图标
        icon_milliliter = 0x1008,  // 毫升选择图标 屏幕已改为0x1008
        icon_maskSelect = 0x1019,  // 面膜选择图标
        icon_wifi = 0x101A,        // wifi强度图标

        icon_testLeftEye = 0x1031,  // 测试点位左眼图标
        icon_testRightEye = 0x1032, // 测试点位右眼图标
        icon_testMouth = 0x1033,    // 测试点位嘴巴图标
    };
    typedef uint16_t ScreenIconAddr_t;

    enum
    {
        text_wifiSSID = 0x2000, // wifi ssid addr
        text_wifiPASS = 0x2100,
        text_deviceID = 0x1060,
        text_firmware = 0x1090,
        text_gui = 0x10B0,
        text_algo = 0x10D0,

        text_testRade = 0x1030,  // 读取成功文本
        text_testWrite = 0x1040, // 读取失败文本
    };
    typedef uint16_t ScreenTextAddr_t;

    enum
    {
        popup_empty_start = 1,
        popup_empty_stop = 2,
        popup_milliliter = 3,
        popup_connnectFile = 4,
        popup_connectSucceed = 5,
    };
    typedef uint8_t ScreenPushButton;

    typedef struct
    {
        uint16_t mask_type;  // 面膜类型 0小面膜，1大面膜，2眼膜，3颈膜
        uint8_t milliliters; // 毫升数
        uint8_t depth;       // 深浅选择 0浅度，1深度
        uint8_t runState;    // 运行状态，0停止运行，1基础夜，2滴液，3排空，4归零
    } ScreenConfig_t;
}

namespace HAL
{
    class UartScreen
    {
    private:
        uint8_t txbuff[256]; // 一包按240字节数据发送，最大使用248字节空间
        uint8_t rxbuff[256];

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

    public:
        UartScreen(MainBoard *mb);
        ~UartScreen();

        void init();
        void reset();

        bool readDataFormRam(uint16_t regAddr, int readLen);
        bool readDataFormFlash(uint16_t flashAddr, int readLen);

        void handle();
        void button_handle(uint16_t addr, uint16_t value);
        void upgrade();

    public:
        uint8_t *getAddrData(uint16_t addr, uint16_t len);
        int getAddrData(uint16_t addr);

        void writeAddrData(uint16_t addr, uint8_t *pdata, uint16_t len);

        void updateIcon(uint16_t addr, uint16_t value);
        void pushTheButton(uint8_t pushNu);
        void dispVersion(char *device_id, char *firmware, char *software);

        String getWifiSSID();
        String getWifiPASS();

        void updataRSSI();

    protected:
        void clearCache();
        uint16_t sendData(uint8_t *buf, uint16_t len);
        bool sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len, bool checkFlag = true, int retry = 3);
        void sendFrameNotCheck(uint8_t cmd, uint8_t *pbuf1, uint16_t len1, uint8_t *pbuf2 = nullptr, uint16_t len2 = 0);
        uint16_t readData(uint8_t *buf, uint16_t len, uint16_t timeOut = 2000);

        bool writeFileToRam(File &file, uint16_t fileSize, uint16_t regAddr); // 一次最大写入32kb
        bool saveToFlash(uint16_t flashAddr, uint16_t ramAddr);
        bool writeFile(const char *filePath, uint8_t saveID);

        bool checkReceiveFrame(uint8_t *buf, uint16_t len);
        bool check(CheckType_t type, uint16_t rlen = 8); // ack 校验默认响应长度为8

    private:
        ScreenConfig_t scrConfig;
        MainBoard *board;
    };
}

#endif