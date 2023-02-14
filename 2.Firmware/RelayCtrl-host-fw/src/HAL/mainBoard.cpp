#include "mainBoard.h"
#include "APP/update/update.h"
#include "ymodem_host.h"

#include "FFat.h"

using namespace HAL;

static int send_data_cb(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    // return (uint16_t)usb_send_data(buf, len);

    return MB_SERIAL.write(buf, len);
}

static int read_data_cb(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    uint16_t rlen = 0;
    unsigned long time = millis();

    do
    {
        if (MB_SERIAL.available())
        {
            rlen += MB_SERIAL.read(&buf[rlen], len);
        }
    } while ((rlen < len) && (millis() - time < timeOut));

    return rlen;
}

static void clearCache_cb()
{
    /* read out all buffer data */
    while (MB_SERIAL.read() >= 0)
        ;
}

void MainBoard::init()
{
    // usb_init();
    MB_SERIAL.begin(115200, SERIAL_8N1, MB_UART_RX_PIN, MB_UART_TX_PIN);
}

int MainBoard::sendData(uint8_t *buf, uint16_t len)
{
    return send_data_cb(buf, len, 0);
}

int MainBoard::readData(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    return read_data_cb(buf, len, timeOut);
}

void MainBoard::handle()
{
    static uint16_t rlen = 0;

    /* wait usart idle */
    if (MB_SERIAL.available())
    {
        rlen += readData(&rxbuff[rlen], MB_SERIAL.available(), 0);
        return;
    }

    if (rlen != 0)
    {
        Serial.printf("[mb] read data len=%d :%.*s \r\n", rlen, rlen, rxbuff);
        rlen = 0;
    }

    this->upgrade();
}

bool MainBoard::upgrade()
{
    /* check updata */
    if (!updateFlag)
        return false;

    /* slave app check string "upgrade" go into bootload program */
    sendData((uint8_t *)"upgrade", 7);

    Serial.println("[mb] Start upgrade.");

    YmodemConfig_t config = {
        .sendData_callback = send_data_cb,
        .readData_callback = read_data_cb,
    };

    YmodemHost ym(&config);

    bool result = ym.sendFile(FFat, "/mainboard.bin");
    ym.get_log();

    setUpgrade(false);

    return result;
}
