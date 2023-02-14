#include "ftpClient.h"
#include "HAL/HAL.h"

#include <WiFi.h>
#include <HTTPClient.h>

bool ftpClient::connectWiFi(String ssid, String pass)
{
    Serial.println("");
    Serial.print("WiFi Connecting: ");
    Serial.print(ssid.c_str());
    Serial.print("@");
    Serial.println(pass.c_str());

    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_STA);

    WiFi.begin(ssid.c_str(), pass.c_str());
    WiFi.setAutoConnect(true);

    uint16_t timeout = 0;
    do
    {
        delay(500);
        Serial.print(".");
    } while ((WiFi.status() != WL_CONNECTED) && (++timeout < 20)); // 10s未连接则超时退出

    if (WiFi.status() != WL_CONNECTED)
    {
        log_e("wifi connect failed");
        return false;
    }

    Serial.println("");
    Serial.println("wifi connect succeed!");
    Serial.printf("MAC address: %s \r\n", WiFi.macAddress().c_str());
    Serial.printf("Obtained IP address: ");
    Serial.println(WiFi.localIP());

    return true;
}

bool ftpClient::isConnected()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[error] Please check the wifi connection!");
        return false;
    }
    return true;
}

bool ftpClient::getFileToFlash(String path, String fileName, String url)
{
    HTTPClient http;

    if (!isConnected())
    {
        return false;
    }

    /* 在路径末尾加上"/" */
    if (path.substring(path.length() - 1, path.length()) != "/")
    {
        path += "/";
    }
    String storage_path = path + fileName;

    Serial.println("[HTTP] begin...");
    http.begin(url + fileName);

    Serial.println("[HTTP] GET...");
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        Serial.printf("[HTTP] GET... code: %d\r\n", httpCode);

        if (httpCode == HTTP_CODE_OK)
        {
            // get length of document (is -1 when Server sends no Content-Length header)
            int len = http.getSize();
            if (len == -1)
            {
                Serial.println("url no info");
                return true;
            }

            uint32_t ptrlen = len;

            // // create buffer for read
            // uint8_t buff[1024] = {0};
            Serial.print("Start download file: ");
            Serial.print(fileName);
            Serial.println("");

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
    }
    else
    {
        Serial.printf("[HTTP] GET...failed, error: %s\r\n", http.errorToString(httpCode).c_str());
        return false;
    }

    http.end();
    return true;
}

ftpClient ftp;