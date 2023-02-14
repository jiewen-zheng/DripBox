#include <Arduino.h>
#include <FFat.h>

#include "../../ymodem_host.h"

static int send_data_cb(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    return Serial.write(buf, len);
}

static int read_data_cb(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    uint16_t rlen = 0;
    unsigned long time = millis();

    do
    {
        if (Serial.available())
        {
            rlen += Serial.read(&buf[rlen], len);
        }
    } while ((rlen < len) && (millis() - time < timeOut));

    return rlen;
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    if (!FFat.begin(true))
    {
        Serial.println("FFat Mount Failed");
        return;
    }

    YmodemConfig_t config = {
        .sendData_callback = send_data_cb,
        .readData_callback = read_data_cb,
    };

    YmodemHost ym(&config);

    ym.sendFile(FFat, "/mainboard.bin");
    ym.get_log();
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(1000);
}