#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "ftpClient.h"
#include "HAL/HAL.h"
#include "HAL/uncompress.h"

/* 存储post数据包 */
DynamicJsonDocument doc(4096); // heap
Uncompress uncomp;

bool FtpClient::checkConnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[error] Please check the wifi connection!");
        return false;
    }
    return true;
}

String FtpClient::reqServer(String url, String msg)
{
    if (!checkConnect())
    {
        return "";
    }

    HTTPClient http;

    http.begin(url);
    // http.addHeader("Content-Type", "application/json");

    //    http.addHeader("Accept", "*/*");
    // start connection and send HTTP header
    Serial.println("[http] post...");
    int code = http.POST(msg);

    Serial.printf("http post code: %d \r\n", code);
    if (code <= 0)
    {
        Serial.printf("[http] post... failed, error: %s.\r\n", http.errorToString(code).c_str());
        http.end();
        return "";
    }

    if (code == HTTP_CODE_OK)
    {
        String read_msg = http.getString();
        http.end();
        return read_msg;
    }

    http.end();
    return "";
}

bool FtpClient::getFileToFlash(String path, String fileName, String url)
{
    HTTPClient http;

    if (!checkConnect())
    {
        return false;
    }

    /* 在路径末尾加上"/" */
    if (path.substring(path.length() - 1, path.length()) != "/")
    {
        path += "/";
    }
    String storage_path = path + fileName;

    http.begin(url);

    Serial.println("[http] get...");
    int code = http.GET();
    Serial.printf("[http] get... code: %d\r\n", code);
    if (code <= 0)
    {
        Serial.printf("[http] get... failed, error: %s\r\n", http.errorToString(code).c_str());
        http.end();
        return false;
    }

    if (code == HTTP_CODE_OK)
    {
        // get length of document (is -1 when Server sends no Content-Length header)
        int len = http.getSize();
        if (len == -1)
        {
            http.end();
            Serial.println("url no info");
            return true;
        }

        uint32_t ptrlen = len;

        Serial.printf("[http] Start download file: \"%s\".\r\n", fileName.c_str());

        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();

        // read all data form server
        while (http.connected() && (len > 0))
        {
            // get available data size
            size_t size = stream->available();

            double ptr = ((double)(ptrlen - len) / ptrlen);
            Serial.printf("downloading... (%d %%)\r\n", (uint8_t)(ptr * 100));

            if (size)
            {
                // read up to 512 byte
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                // write it to flash
                ffat.appendFile(storage_path.c_str(), buff, c);

                if (len > 0)
                {
                    len -= c;
                }
            }
        }

        if (len > 0)
        {
            Serial.println("[HTTP] connection closed, file download failed.");
            http.end();
            return false;
        }
        else
        {
            Serial.println("[HTTP] file download succeed.");
        }
    }

    http.end();
    return true;
}

bool FtpClient::getUpdateFileUrl(PackageInfo_t *info, String req_url)
{

    String msg = reqServer(req_url, "");

    Serial.println(msg);
    if (msg == "")
    {
        Serial.println("[ftp] req server no data");
        return false;
    }

    doc.clear();
    DeserializationError code = deserializeJson(doc, msg);
    if (code)
    {
        Serial.printf("[error]: tcp json deserialize failed: %s\r\n", code.c_str());
        return false;
    }
    JsonObject data = doc["data"];

    memset(info, 0, sizeof(PackageInfo_t));

    info->fileName = data["fileName"].as<String>();
    Serial.println("name: " + info->fileName);
    info->fileType = data["fileType"].as<int>();
    Serial.println("type: " + String(info->fileType));
    info->fileUrl = data["fileUrl"].as<String>();
    Serial.println("url: " + info->fileUrl);
    info->phoneModel = data["phoneModel"].as<String>();
    Serial.println("model: " + info->phoneModel);
    info->version = data["version"].as<int>();
    Serial.println("version: " + String(info->version));

    return true;
}

FtpState_e FtpClient::getFirmware(int now_ver)
{
    // get firmware download url
    if (!getUpdateFileUrl(&firmware_info, FIRMWARE_REQ_URL))
    {
        Serial.println("[update] get firmware failed");
        return SERVER_ERROR;
    }

    if (firmware_info.fileType != 0 || firmware_info.phoneModel != "ROTEX-Z003")
    {
        Serial.println("[update] get firmware message error");
        return SERVER_ERROR;
    }

    if (firmware_info.version <= now_ver)
    {
        return NOT_UPDATE;
    }

    return REQ_OK;
}

bool FtpClient::firmwareDownload()
{
    ffat.deleteAllFile("/firmware");

    // downlowe file to root directory
    bool code = getFileToFlash("/", firmware_info.fileName, firmware_info.fileUrl);

    if (!code)
    {
        return false;
    }

    // Check whether it is a .zip file
    char *ptr = strstr(firmware_info.fileName.c_str(), ".zip");

    if (ptr)
    {
        String zip_path = "/" + firmware_info.fileName;
        // unzip file to firmware directory
        if (uncomp.unzipFile(zip_path.c_str(), "/firmware/"))
        {
            // delete the .zip file, after unzip it
            ffat.deleteFile(zip_path.c_str());
        }
        else
        {
            false;
        }
    }

    return true;
}

int FtpClient::getFirmwareVersion()
{
    return firmware_info.version;
}

FtpState_e FtpClient::getSoftware(int now_ver)
{

    if (!getUpdateFileUrl(&software_info, SOFTWARE_REQ_URL))
    {
        Serial.println("[update] get software failed");
        return SERVER_ERROR;
    }

    if (software_info.fileType != 1 || software_info.phoneModel != "ROTEX-Z003")
    {
        Serial.println("[update] get software message error");
        return SERVER_ERROR;
    }

    if (software_info.version <= now_ver)
    {
        return NOT_UPDATE;
    }

    // bool code = getFileToFlash("/", software_info.fileName, software_info.fileUrl);

    // if (!code)
    // {
    //     return false;
    // }
    return REQ_OK;
}

String FtpClient::getSoftwareURL()
{
    return software_info.fileUrl;
}

int FtpClient::getSoftwareVersion()
{
    return software_info.version;
}
