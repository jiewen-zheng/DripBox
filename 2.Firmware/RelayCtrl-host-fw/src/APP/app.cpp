#include "app.h"

#include "HAL/HAL.h"
#include "HAL/uncompress.h"
#include "update/update.h"
#include "ftpClient/ftpClient.h"

#include <FFat.h>
#include <ArduinoJson.h>

Uncompress uncomp;

static char debugBuff[128];

void app_init()
{
    ftp.connectWiFi("monster", "sunflower6697");

    ffat.listDir("/", 0);

    if (!ffat.findDir("/config"))
    {
        Serial.println("creat config dir");
        ffat.createDir("/config");
    }

    // if (!ffat.findFile("config.json"))
    // {
    //     ftp.getFileToFlash("/", "config.json");
    // }
}

void app_loop()
{
    // ftp.getFileUrl();
    //  scr.handle();
    //  board.handle();
}

void serialEvent()
{
    if (Serial.available() > 0)
    {
        memset(debugBuff, 0, sizeof(debugBuff));
        uint16_t rd_len = Serial.read(debugBuff, sizeof(debugBuff)); // read data;

        // MB_SERIAL.write(debugBuff, rd_len);
        if (memcmp(debugBuff, "help", 4) == 0)
        {
            Serial.println("available cmd:");
            Serial.println("[list] [delete] [scrup] [mbup] [mbsend] [seturl]");
        }
        else if (memcmp(debugBuff, "list", 4) == 0)
        {
            ffat.listDir("/", 0);
        }
        else if (memcmp(debugBuff, "seturl", 6) == 0)
        {
            String url = String(&debugBuff[7]);
            Serial.print(url);
            ftp.setServerUrl(url);
        }
        else if (memcmp(debugBuff, "delete", 6) == 0)
        {
            String file = String(&debugBuff[7]);
            Serial.printf("delete file: %s \r\n", file.c_str());
            ffat.deleteFile(file.c_str());
        }
        else if (memcmp(debugBuff, "scrup", 5) == 0)
        {
            update_checkFile();
            scr.upgrade();
        }
        else if (memcmp(debugBuff, "mbup", 4) == 0)
        {
            update_checkFile();
            board.setUpgrade(true);
        }
        else if (memcmp(debugBuff, "mbsend ", 7) == 0)
        {
            board.sendData((uint8_t *)(debugBuff + 7), rd_len - 7);
        }
        else if (memcmp(debugBuff, "unzip", 5) == 0)
        {

            String file = String(&debugBuff[6]);
            uncomp.unzipFile(file.c_str(), "/");
        }
        else if (memcmp(debugBuff, "write", 5) == 0)
        {
            char name[32];
            char *data = strchr(&debugBuff[6], ' ');
            data += 1;
            int dataLen = strlen(data);
            int nameLen = strlen(&debugBuff[6]) - dataLen - 1;
            memcpy(name, &debugBuff[6], nameLen);

            ffat.writeFile(name, data, dataLen);
        }
        else if (memcmp(debugBuff, "read", 4) == 0)
        {

            String file = String(&debugBuff[5]);
            ffat.readFile(file.c_str());
        }
        else if (memcmp(debugBuff, "json", 4) == 0)
        {
            String file = "";
            if (debugBuff[6] != '/')
            {
                file += "/";
            }

            file += String(&debugBuff[5]);

            char *buff = (char *)malloc(1024);
            if (!buff)
            {
                return;
            }

            ffat.readFile(file.c_str(), (uint8_t *)buff, 1024);

            StaticJsonDocument<1024> doc;
            deserializeJson(doc, buff);
            free(buff);
            serializeJson(doc, Serial);
        }
        else
        {
            Serial.printf("unknown: \"%s\"", debugBuff);
            Serial.println("usage: \"help\" view help.");
        }
    }
}