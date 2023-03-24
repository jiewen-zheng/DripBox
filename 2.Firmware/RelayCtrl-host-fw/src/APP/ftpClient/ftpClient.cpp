#include <WiFi.h>
#include <HTTPClient.h>

#include "ftpClient.h"
#include "HAL/HAL.h"

// DynamicJsonDocument doc(2048); // heap

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
    Serial.println("msg=");
    Serial.println(msg);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    //    http.addHeader("Accept", "*/*");
    // start connection and send HTTP header
    Serial.println("[http] post...");
    int code = http.POST("");

    if (code <= 0)
    {
        Serial.printf("[http] post... failed, error: %s.\r\n", http.errorToString(code).c_str());
        http.end();
        return "";
    }

    if (code == HTTP_CODE_OK)
    {
        http.end();
        return http.getString();
    }

    http.end();
    return "";
}

bool FtpClient::getFileToFlash(String path, String fileName)
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

    String url = "http://59.110.138.60:8008/File/download?id=1677313300167";
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
        }
        else
        {
            Serial.println("[HTTP] file download succeed.");
        }
    }

    http.end();
    return true;
}
