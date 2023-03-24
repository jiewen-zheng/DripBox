
#include <Arduino.h>
#include <ArduinoJson.h>

#include "update.h"
#include "HAL/HAL.h"

/* 存储post数据包 */
DynamicJsonDocument doc(4096); // heap

// void update_checkFile()
// {
//     ffat.listDir("/", 0);

//     if (!ffat.findFile("13TouchFile.bin"))
//     {
//         ftp.getFileToFlash("/", "13TouchFile.bin");
//     }
//     if (!ffat.findFile("14ShowFile.bin"))
//     {
//         ftp.getFileToFlash("/", "14ShowFile.bin");
//     }
//     if (!ffat.findFile("22_Config.bin"))
//     {
//         ftp.getFileToFlash("/", "22_Config.bin");
//     }
//     if (!ffat.findFile("32_Image.icl"))
//     {
//         ftp.getFileToFlash("/", "32_Image.icl");
//     }

//     if (!ffat.findFile("mainboard.bin"))
//     {
//         ftp.getFileToFlash("/", "mainboard.bin");
//     }
// }

bool Update::getFirmware()
{
    String msg = reqServer(FIRMWARE_REQ_URL, "");

    doc.clear();
    DeserializationError code = deserializeJson(doc, msg);
    if (code)
    {
        Serial.printf("[error]: tcp json deserialize failed: %s\r\n", code.c_str());
        return false;
    }

    memset(&firmware_info, 0, sizeof(PackageInfo_t));

    firmware_info.fileName = doc["fileName"].as<String>();
    firmware_info.fileType = doc["fileType"].as<int>();
    firmware_info.fileUrl = doc["fileUrl"].as<String>();
    firmware_info.phoneModel = doc["phoneModel"].as<String>();
    firmware_info.version = doc["version"].as<int>();

    if (firmware_info.phoneModel != "ROTEX-Z003")
    {
        return false;
    }

    if (firmware_info.fileType != 0)
    {
        return false;
    }

    return true;
}
