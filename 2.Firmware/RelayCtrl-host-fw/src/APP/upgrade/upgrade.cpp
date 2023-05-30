#include "upgrade.h"
#include "../common.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

DynamicJsonDocument configDoc(2048); // heap

void update_started()
{
    Serial.println("CALLBACK: HTTP update process started");
}

void update_finished()
{
    Serial.println("CALLBACK: HTTP update process finished");
}

void update_progress(int cur, int total)
{
    Serial.printf("CALLBACK: HTTP update process at %d of %d bytes...\r\n", cur, total);
}

void update_error(int err)
{
    Serial.printf("CALLBACK: HTTP update fatal error code %d\r\n", err);
}

void firm_update_cb()
{
    upgrade.setUpdateFirmware(true);
}

void soft_update_cb()
{
    upgrade.setUpdateSoftware(true);
}

void saveWifiMsg_cb(const char *ssid, const char *pass)
{
    upgrade.save_wifi_msg(String(ssid), String(pass));
}

Upgrade::Upgrade()
{
    firmware_flag = false;
    firmware_flag = false;
    ftp = new FtpClient();

    fileMsg = {
        .wifi_ssid = "",
        .wifi_pass = "",
        .firm_ver = 1,
        .firm_update = false,
        .soft_ver = 1,
        .soft_update = false,
    };
}

Upgrade::~Upgrade()
{
    p_ffat = nullptr;
    delete ftp;
}

void Upgrade::init(HAL::FlashFFAT *pffat, HAL::UartScreen *pScreen, HAL::MainBoard *pBoard)
{
    if (pffat != nullptr)
        p_ffat = pffat;

    p_screen = pScreen;
    p_board = pBoard;

    p_screen->setUpdateCallback(firm_update_cb, soft_update_cb);
    p_screen->setWiFiCallback(saveWifiMsg_cb);

    p_ffat->listDir("/config", 0);
    if (!p_ffat->findFile("config.json"))
    {
        Serial.println("not config.json");
        configDoc.clear();

        configDoc["wifi_ssid"] = "";
        configDoc["wifi_pass"] = "";
        configDoc["firmware_ver"] = 1;
        configDoc["software_ver"] = 1;
        JsonObject update = configDoc.createNestedObject("update");

        update["firmware"] = 0;
        update["software"] = 0;

        String msg;

        serializeJson(configDoc, msg);
        p_ffat->writeFile("/config/config.json", msg.c_str());
        return;
    }
    else
    {
        checkConfigFile();
    }

    if (!fileMsg.wifi_ssid.isEmpty() && !fileMsg.wifi_pass.isEmpty())
    {
        connectWiFi(fileMsg.wifi_ssid, fileMsg.wifi_pass);
    }

    if (fileMsg.firm_update)
    {
        setUpdateFirmware(true);
    }
}

bool Upgrade::firmUpdateState()
{
    return firmware_flag;
}

void Upgrade::checkConfigFile()
{

    memset(read_buff, 0, sizeof(read_buff));

    size_t len = p_ffat->readFile("/config/config.json", read_buff, sizeof(read_buff));

    configDoc.clear();
    DeserializationError error = deserializeJson(configDoc, read_buff);
    if (error)
    {
        Serial.printf("[error]: config json deserialize failed: %s\r\n", error.c_str());
        return;
    }

    fileMsg.wifi_ssid = configDoc["wifi_ssid"].as<String>();
    fileMsg.wifi_pass = configDoc["wifi_pass"].as<String>();

    fileMsg.firm_ver = configDoc["firmware_ver"].as<int>();
    fileMsg.soft_ver = configDoc["software_ver"].as<int>();

    if (!configDoc.containsKey("update") || configDoc["update"].isNull())
    {
        return;
    }

    JsonObject update = configDoc["update"];

    fileMsg.firm_update = update["firmware"].as<bool>();
    fileMsg.soft_update = update["software"].as<bool>();

    // if (fileMsg.firm_update)
    // {
    //     setUpdateFirmware(true);
    // }
}

void Upgrade::writeConfigFile(ConfigFileMsg_t *msg)
{
    configDoc.clear();

    configDoc["wifi_ssid"] = msg->wifi_ssid;
    configDoc["wifi_pass"] = msg->wifi_pass;
    configDoc["firmware_ver"] = msg->firm_ver;
    configDoc["software_ver"] = msg->soft_ver;
    JsonObject update = configDoc.createNestedObject("update");

    update["firmware"] = msg->firm_update;
    update["software"] = msg->soft_update;

    String str_msg;

    serializeJson(configDoc, str_msg);
    p_ffat->writeFile("/config/config.json", str_msg.c_str());
}

void Upgrade::setUpdateFirmware(bool flag)
{
    firmware_flag = flag;
}

void Upgrade::setUpdateSoftware(bool flag)
{
    software_flag = flag;
}

bool Upgrade::updateScreen()
{
    if (p_screen == nullptr)
    {
        return false;
    }

    int file_count = p_ffat->getFileCount("/firmware");

    FileInfo_t file;
    char *ptr;
    for (size_t i = 0; i < file_count; i++)
    {
        file = p_ffat->getFileInfo(i);
        int num = strtoul(file.name.c_str(), &ptr, 10);

        // 去除不已数字开头的文件
        if (num == 0)
        {
            if (strlen(ptr) == strlen(file.name.c_str()))
            {
                continue;
            }
        }

        String logmsg = "Install ";
        logmsg += file.name;

        p_screen->LogUpgradeMsg(logmsg.c_str());
        if (!p_screen->upgreadFile(file.path.c_str(), num))
        {
            return false;
        }
    }

    p_screen->reset();
    return true;
}

bool Upgrade::updateBoard()
{
    if (p_board == nullptr)
    {
        return false;
    }

    int file_count = p_ffat->getFileCount("/firmware");

    FileInfo_t file;
    char *ptr;
    while (file_count--)
    {
        file = p_ffat->getFileInfo(file_count);
        int num = strtoul(file.name.c_str(), &ptr, 10);

        if (num == 0)
        {
            if (strlen(ptr) == strlen(file.name.c_str()))
            {
                p_screen->LogUpgradeMsg("Install control board firmware");
                return p_board->upgrade(file.path.c_str());
            }
            else
            {
                return true;
            }
        }
    }

    return true;
}

void Upgrade::updateFirmware()
{
    bool up_board = false, up_scr = false;
    uint8_t try_num = 0;

    if (!firmware_flag)
    {
        return;
    }

    FtpState_e state = ftp->getFirmware(fileMsg.firm_ver);

    if (state == NOT_UPDATE)
    {
        displayVersion();
        p_screen->firmware_have_update();
        setUpdateFirmware(false);
        return;
    }

    if (state != REQ_OK)
    {
        setUpdateFirmware(false);
        return;
    }

    p_screen->pageSwitch(10);

    /* save update msg */
    fileMsg.firm_update = true;
    writeConfigFile(&fileMsg);

    p_screen->LogUpgradeMsg("downloading...");
    if (!ftp->firmwareDownload())
    {
        p_screen->LogUpgradeMsg("firmware download failed");
        p_screen->pageSwitch(1);
        setUpdateFirmware(false);
        return;
    }

    /* update */

    try_num = 0;
    do
    {
        up_board = updateBoard();
    } while (!up_board && ++try_num < UPGRADE_MAX_RETRY_NUM);
    // up_board = true;

    try_num = 0;
    do
    {
        up_scr = updateScreen();
    } while (!up_scr && ++try_num < UPGRADE_MAX_RETRY_NUM);
    // up_scr = true;

    /* clear update msg */

    if (up_board && up_scr)
    {
        fileMsg.firm_ver = ftp->getFirmwareVersion();
        fileMsg.firm_update = false;
        writeConfigFile(&fileMsg);
    }

    p_screen->pageSwitch(1);
    setUpdateFirmware(false);
}

void Upgrade::updateSotfware()
{
    if (!software_flag)
    {
        return;
    }
    WiFiClient client;

    setUpdateSoftware(false);

    FtpState_e state = ftp->getSoftware(fileMsg.soft_ver);

    if (state == NOT_UPDATE)
    {
        displayVersion();
        p_screen->software_have_update();
        return;
    }

    if (state != REQ_OK)
    {
        return;
    }

    p_screen->pageSwitch(10);

    String url = ftp->getSoftwareURL();
    p_screen->LogUpgradeMsg("update software...");

    if (url != "")
    {
        httpUpdate.onStart(update_started);
        httpUpdate.onEnd(update_finished);
        httpUpdate.onProgress(update_progress);
        httpUpdate.onError(update_error);
        httpUpdate.rebootOnUpdate(false);

        t_httpUpdate_return ret = httpUpdate.update(client, url);

        Serial.printf("update ret: %d \r\n", ret);

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            p_screen->software_have_update();
            fileMsg.soft_ver = ftp->getSoftwareVersion();
            fileMsg.soft_update = false;
            writeConfigFile(&fileMsg);
            break;
        }
    }
    p_screen->pageSwitch(1);
    ESP.restart();
}

void Upgrade::displayVersion()
{
    HAL::VersionMsg_t msg;

    int major, minor, patch;

    msg.dev = "Rotex-Z003";

    major = fileMsg.firm_ver / 100;
    minor = (fileMsg.firm_ver % 100) / 10;
    patch = fileMsg.firm_ver % 10;
    msg.firm = "Firmware-V" + String(major) + "." + String(minor) + "." + String(patch);

    major = fileMsg.soft_ver / 100;
    minor = (fileMsg.soft_ver % 100) / 10;
    patch = fileMsg.soft_ver % 10;
    msg.soft = "Software-V" + String(major) + "." + String(minor) + "." + String(patch);

    p_screen->setVerMsg(&msg);
    p_screen->updateVerMsg();
}

void Upgrade::save_wifi_msg(String ssid, String pass)
{
    fileMsg.wifi_ssid = ssid;
    fileMsg.wifi_pass = pass;
    writeConfigFile(&fileMsg);
}

Upgrade upgrade;