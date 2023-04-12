#include "mainBoard.h"
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

void HAL::MainBoard::sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len)
{
    uint8_t *send_buf = txbuff;

    uint16_t f_len = len + 3;

    send_buf[0] = 0x5A;
    send_buf[1] = f_len >> 8;
    send_buf[2] = f_len;
    send_buf[3] = cmd;

    memcpy(send_buf + 4, buf, len);

    /* crc check is "data + cmd" */
    uint16_t crc = ym_crc16(send_buf + 3, len + 1);
    send_buf[len + 4] = crc >> 8;
    send_buf[len + 5] = crc;

    sendData(send_buf, len + 6);
}

int MainBoard::readData(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    return read_data_cb(buf, len, timeOut);
}

bool HAL::MainBoard::checkReceiveData(uint8_t *buf, uint16_t len)
{
    /* check head */
    uint8_t head = buf[0];
    if (head != 0x5A)
    {
        return false;
    }

    /* get data len */
    frameCheck.dataLen = ((buf[1] << 8) | buf[2]) - 3; // lost crc and cmd.

    /* check crc */
    frameCheck.crc = (buf[len - 2] << 8) | buf[len - 1];
    uint16_t crc = ym_crc16(&buf[3], frameCheck.dataLen + 1); // cmd + data

#if COMM_CRC_EN
    if (crc != frameCheck.crc)
    {
        return false;
    }
#endif

    /* get data */
    frameCheck.frameLen = len;
    frameCheck.cmd = buf[3];
    frameCheck.data = &buf[4];

    return true;
}

bool HAL::MainBoard::checkCmdACK(const char *ack)
{
    uint8_t buf[2];

    int len = readData(buf, 2, 2000);
    if (len != 2 || memcmp(buf, ack, 2) != 0)
    {
        return false;
    }

    return true;
}

void HAL::MainBoard::data_execute(uint8_t cmd, uint8_t *data, uint16_t len)
{
    switch (cmd)
    {
    case 0x89:
        saveDeviceState(data, len);
        break;

    case 0x90:
        offset.read = true;
        offset.x = data[0];
        offset.y = data[1];
        offset.z = data[2];
        break;

    default:
        break;
    }
}

void HAL::MainBoard::saveDeviceState(uint8_t *data, uint16_t len)
{
    if (len < 10)
        return;

    DeviceState_t *dev = getDeviceState();

    dev->run_state = data[0];
    dev->progress = data[1];
    dev->uv_light = data[2];
    dev->water_pump = data[3];
    dev->stop_state = data[4];
    dev->x_zero = data[5];
    dev->y_zero = data[6];
    dev->z_zero = data[7];
    dev->water_too_low = data[8];
    dev->error = data[9];
}

DeviceState_t *HAL::MainBoard::getDeviceState()
{
    return &deviceState;
}

void HAL::MainBoard::reqDeviceData(uint16_t time)
{
    static unsigned long inter = millis();

    if (millis() - inter < time)
    {
        return;
    }
    inter = millis();

    uint8_t data = 0x01;
    sendFrame(0x89, &data, 1);
}

bool HAL::MainBoard::baseLiquid(bool onOff, uint8_t mask, uint8_t mil, uint8_t depth)
{
    uint8_t data = mask * 2 + depth + 1;

    if (onOff)
    {
        sendFrame(0x81, &data, 1);
    }
    else
    {
        gotoZero();
    }

    return checkCmdACK();
}

bool HAL::MainBoard::dropLiquid(bool onOff, uint8_t mask, uint8_t mil, uint8_t depth)
{
    uint8_t data = mask * 4 + mil - 1;

    if (onOff)
        sendFrame(0x82, &data, 1);
    else
        gotoZero();

    return checkCmdACK();
}

bool HAL::MainBoard::empty(bool onOff)
{
    uint8_t data[1] = {0x01};

    if (onOff)
        sendFrame(0x83, data, 1);
    else
        gotoZero();

    return checkCmdACK();
}

void HAL::MainBoard::gotoZero()
{
    uint8_t data[1] = {0x01};

    sendFrame(0x85, data, 1);
}

bool HAL::MainBoard::light(bool onOff)
{
    uint8_t data[1] = {0x01};
    data[0] = onOff;

    sendFrame(0x86, data, 1);

    return checkCmdACK();
}

bool HAL::MainBoard::moveTestPoint(uint8_t point)
{
    uint8_t data = point;

    sendFrame(0x92, &data, 1);

    return checkCmdACK();
}

bool HAL::MainBoard::setOffset(uint8_t select)
{
    uint8_t pos = 1;
    uint8_t neg = 0xff;

    switch (select)
    {
    case 1:
        sendFrame(0x94, &neg, 1);
        break;

    case 2:
        sendFrame(0x94, &pos, 1);
        break;

    case 3:
        sendFrame(0x93, &neg, 1);
        break;

    case 4:
        sendFrame(0x93, &pos, 1);
        break;

    default:
        break;
    }

    return checkCmdACK();
}

bool HAL::MainBoard::moveTest(bool onOff)
{
    uint8_t data = 1;

    if (onOff)
        sendFrame(0x96, &data, 1);
    else
        gotoZero();

    return checkCmdACK();
}

bool HAL::MainBoard::readOffset()
{
    uint8_t data = 1;

    sendFrame(0x90, &data, 1);

    bool read = offset.read;
    offset.read = false;

    return read != false;
}

String HAL::MainBoard::getOffsetMsg()
{
    String msg = "x:" + String(offset.x) + " y:" + String(offset.y) + " z:" + String(offset.z);
    return msg;
}

bool HAL::MainBoard::writeOffset()
{
    uint8_t data[3];

    data[0] = offset.x;
    data[1] = offset.y;
    data[2] = offset.z;

    sendFrame(0x91, data, 3);

    return checkCmdACK();
}

void MainBoard::handle()
{
    reqDeviceData();

    dataPack_handle();
}

void HAL::MainBoard::dataPack_handle()
{
    static uint16_t rlen = 0;

    /* wait usart idle */
    if (MB_SERIAL.available())
    {
        rlen += readData(&rxbuff[rlen], MB_SERIAL.available(), 0);
        return;
    }
    if (rlen == 0)
    {
        return;
    }

    // debug
    Serial.printf("[mb] read data len=%d :%.*s \r\n", rlen, rlen, rxbuff);

    for (uint16_t i = 0; i < rlen; i++)
    {
        Serial.printf("%x ", rxbuff[i]);
    }
    Serial.println("");
    // debug

    /* check board frame */
    if (!checkReceiveData(rxbuff, rlen))
    {
        rlen = 0;
        return;
    }
    rlen = 0;

    data_execute(frameCheck.cmd, frameCheck.data, frameCheck.dataLen);
}

bool MainBoard::upgrade(const char *filePath)
{
    /* slave app check string "upgrade" go into bootload program */
    sendData((uint8_t *)"upgrade", 7);

    Serial.println("[mb] Start upgrade.");
    Serial.println("[mb] upgread file : " + String(filePath));

    YmodemConfig_t config = {
        .sendData_callback = send_data_cb,
        .readData_callback = read_data_cb,
    };

    YmodemHost ym(&config);

    bool result = ym.sendFile(FFat, filePath);
    ym.get_log();

    if (result)
    {
        Serial.println("[mb] upgrade success.");
    }
    else
    {
        Serial.println("[mb] upgrade failed.");
    }

    return result;
}
