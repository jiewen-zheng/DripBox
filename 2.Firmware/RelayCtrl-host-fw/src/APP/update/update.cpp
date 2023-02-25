
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "update.h"
#include "HAL/HAL.h"
#include "APP/ftpClient/ftpClient.h"

/* 存储post数据包 */


void update_checkFile()
{
    ffat.listDir("/", 0);

    if (!ffat.findFile("13TouchFile.bin"))
    {
        ftp.getFileToFlash("/", "13TouchFile.bin");
    }
    if (!ffat.findFile("14ShowFile.bin"))
    {
        ftp.getFileToFlash("/", "14ShowFile.bin");
    }
    if (!ffat.findFile("22_Config.bin"))
    {
        ftp.getFileToFlash("/", "22_Config.bin");
    }
    if (!ffat.findFile("32_Image.icl"))
    {
        ftp.getFileToFlash("/", "32_Image.icl");
    }

    if (!ffat.findFile("mainboard.bin"))
    {
        ftp.getFileToFlash("/", "mainboard.bin");
    }
}

bool Update::checkConnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    return true;
}

bool Update::reqServer(String url, String msg)
{
    if (!checkConnect())
    {
        return false;
    }

    HTTPClient http;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    // start connection and send HTTP header
    int code = http.POST(msg);

    if (code > 0)
    {
        Serial.printf("[http] post.code: %d \r\n", code);



    }
    return false;
}
