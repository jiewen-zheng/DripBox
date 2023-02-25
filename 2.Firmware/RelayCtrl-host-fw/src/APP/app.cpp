#include "app.h"

#include "HAL/HAL.h"
#include "HAL/uncompress.h"
#include "update/update.h"
#include "ftpClient/ftpClient.h"

#include <FFat.h>

Uncompress uncomp;

static char debugBuff[128];

void app_init()
{
    ftp.connectWiFi("monster", "sunflower6697");

    ffat.listDir("/", 0);
    if (!ffat.findFile("hello.zip"))
    {
        ftp.getFileToFlash("/", "hello.zip");
    }

    ffat.readFile("/hello.txt");
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
            String file = "";
            if (debugBuff[7] != '/')
            {
                file += "/";
            }

            file += String(&debugBuff[7]);
            Serial.print("delete file name: ");
            Serial.println(file);
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
            String file = "";
            if (debugBuff[6] != '/')
            {
                file += "/";
            }

            file += String(&debugBuff[6]);
            uncomp.unzipFile(file.c_str(), "/");
        }
        else if (memcmp(debugBuff, "read", 4) == 0)
        {
            String file = "";
            if (debugBuff[6] != '/')
            {
                file += "/";
            }

            file += String(&debugBuff[5]);

            ffat.readFile(file.c_str());
        }
        else
        {
            Serial.printf("unknown: \"%s\"", debugBuff);
            Serial.println("usage: \"help\" view help.");
        }
    }
}