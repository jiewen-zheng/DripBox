#ifndef __UPGRADE_H
#define __UPGRADE_H

#include "HAL/HAL.h"
#include "APP/ftpClient/ftpClient.h"

#define UPGRADE_MAX_RETRY_NUM   3

class Upgrade
{
private:
    bool firmware_flag;
    bool software_flag;

    uint8_t read_buff[2048];

    // file
    typedef struct
    {
        String wifi_ssid;
        String wifi_pass;
        int firm_ver;
        bool firm_update;
        int soft_ver;
        bool soft_update;
    } ConfigFileMsg_t;
    ConfigFileMsg_t fileMsg;

public:
    Upgrade();
    ~Upgrade();

    void init(HAL::FlashFFAT *pffat, HAL::UartScreen *pScreen, HAL::MainBoard *pBoard);

    bool firmUpdateState();

    void setUpdateFirmware(bool flag);
    void setUpdateSoftware(bool flag);

    void updateFirmware();
    void updateSotfware();

    void displayVersion();

    void save_wifi_msg(String ssid, String pass);

protected:
    bool updateScreen();
    bool updateBoard();

    void checkConfigFile();
    void writeConfigFile(ConfigFileMsg_t *msg);

private:
    HAL::FlashFFAT *p_ffat;
    HAL::UartScreen *p_screen;
    HAL::MainBoard *p_board;
    FtpClient *ftp;
};

extern Upgrade upgrade;
#endif