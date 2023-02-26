
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "ftpClient.h"
#include "HAL/HAL.h"

DynamicJsonDocument doc(2048); // heap

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

bool ftpClient::checkConnect()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[error] Please check the wifi connection!");
        return false;
    }
    return true;
}

void ftpClient::setServerUrl(String url)
{
    serverURL = url;
}

bool ftpClient::getFileUrl()
{
    // StaticJsonDocument<256> doc;

    // JsonObject body = doc.createNestedObject("body");

    // body["phoneModel"] = "ROTEX-Z003"; // 设备型号
    // body["fileType"] = "0";            // 固件使用0，软件使用1

    String str = "http://59.110.138.60:8008/fileInfo/selectOne?phoneModel=ROTEX-Z003&fileType=0";

    reqServer(str, "");
    return true;
}

bool ftpClient::reqServer(String url, String msg)
{
    if (!checkConnect())
    {
        return false;
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
        return false;
    }

    if (code == HTTP_CODE_OK)
    {
        doc.clear();
        // 反序列化Json
        DeserializationError error = deserializeJson(doc, http.getString());
        if (error)
        {
            Serial.printf("[error]: http json deserialize failed: %s.\r\n", error.c_str());
            http.end();
            return false;
        }

        serializeJsonPretty(doc, Serial); // debug
    }

    http.end();
    return true;
}

bool ftpClient::getFileToFlash(String path, String fileName)
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

ftpClient ftp;