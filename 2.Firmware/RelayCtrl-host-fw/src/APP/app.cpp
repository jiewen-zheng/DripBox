#include "app.h"

#include "HAL/HAL.h"
#include "update/update.h"
#include "ftpClient/ftpClient.h"

static char debugBuff[128];

void app_init()
{
    ftp.connectWiFi("RDYK", "Rotex2016");
}

void app_loop()
{
    
    scr.handle();
    board.handle();
}

void serialEvent()
{
    if (Serial.available() > 0)
    {
        memset(debugBuff, 0, sizeof(debugBuff));
        uint16_t rd_len = Serial.read(debugBuff, sizeof(debugBuff)); // read data;

        // MB_SERIAL.write(debugBuff, rd_len);
        if (memcmp(debugBuff, "list", 4) == 0)
        {
            ffat.listDir("/", 0);
        }
        else if (memcmp(debugBuff, "delete", 6) == 0)
        {
            String file = String(&debugBuff[7]);
            Serial.print("delete file name: ");
            Serial.println(file);
            ffat.deleteFile(file.c_str());
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
    }
}