#include <FFat.h>
#include <ArduinoJson.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main_app.h"
#include "common.h"
#include "HAL/HAL.h"
#include "upgrade/upgrade.h"
#include "ftpClient/ftpClient.h"

static char debugBuff[32];

TaskHandle_t handleTaskUptate;
void TaskUpdate(void *parameter)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for (;;)
    {
        upgrade.updateFirmware();
        delay(50);
    }
}

void app_init()
{
    // connectWiFi("monster", "sunflower6697");
    upgrade.init(&ffat, &screen, &board);

    delay(2000);
    upgrade.displayVersion();

    xTaskCreate(
        TaskUpdate,
        "updateTask",
        10240,
        nullptr,
        configMAX_PRIORITIES - 1,
        &handleTaskUptate);

    xTaskNotifyGive(handleTaskUptate);
}

void app_loop()
{
    // Serial.println("software update");
    // delay(500);
    if (!upgrade.firmUpdateState())
    {
        board.handle();
        screen.handle();
    }

    upgrade.updateSotfware();
}

void serialEvent()
{
    if (Serial.available() > 0)
    {
        uint16_t rd_len = 0;
        memset(debugBuff, 0, sizeof(debugBuff));
        do
        {
            rd_len += Serial.read(&debugBuff[rd_len], sizeof(debugBuff)); // read data;
        } while (Serial.available());

        // MB_SERIAL.write(debugBuff, rd_len);
        if (debugBuff[0] == 0x5a && debugBuff[1] == 0xa5)
        {
            screen.sendFrame(debugBuff[2], (uint8_t *)&debugBuff[3], rd_len - 3);
            return;
        }

        if (debugBuff[0] == 0x5a)
        {
            board.sendFrame(debugBuff[1], (uint8_t *)&debugBuff[2], rd_len - 2);
            return;
        }

        if (memcmp(debugBuff, "format", 6) == 0)
        {
            FFat.end();
            FFat.format();
            ESP.restart();
        }
        else if (memcmp(debugBuff, "list", 4) == 0)
        {
            String dir = String(&debugBuff[5]);
            ffat.listDir(dir.c_str(), 0);
        }
        else if (memcmp(debugBuff, "delete", 6) == 0)
        {
            String file = String(&debugBuff[7]);
            Serial.printf("delete file: %s \r\n", file.c_str());
            ffat.deleteFile(file.c_str());
        }
        else if (memcmp(debugBuff, "remove", 6) == 0)
        {
            String path = String(&debugBuff[7]);
            ffat.deleteAllFile(path.c_str());
        }
        else if (memcmp(debugBuff, "firmware", 8) == 0)
        {
            upgrade.setUpdateFirmware(true);
        }
        else if (memcmp(debugBuff, "software", 8) == 0)
        {
            upgrade.setUpdateSoftware(true);
        }
        else if (memcmp(debugBuff, "mbsend", 6) == 0)
        {
            board.sendData((uint8_t *)(debugBuff + 7), rd_len - 7);
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
        else
        {
            Serial.printf("unknown: \"%s\"", debugBuff);
        }
    }
}