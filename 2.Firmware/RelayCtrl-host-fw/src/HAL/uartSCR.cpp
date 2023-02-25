#include "uartSCR.h"
#include "FFat.h"

extern "C"
{
#include "Utils/src_crc16.h"
}

using namespace HAL;

void UART_SCR::init()
{

    SCR_SERIAL.begin(115200, SERIAL_8N1, SCR_UART_RX_PIN, SCR_UART_TX_PIN);

    // uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);
    // if (!buf)
    // {
    //     log_e("scr uart tx buff malloc failed");
    //     return;
    // }

    // txbuff = buf;
}

void UART_SCR::reset()
{
    uint8_t buf[6] = {0x00, 0x04, 0x55, 0xAA, 0x5A, 0XA5};

    Serial.println("[scr] reset");
    sendFrame(0x82, buf, 6);
}

uint16_t UART_SCR::sendData(uint8_t *buf, uint16_t len)
{
    // Serial.print("send data is:");
    // for (uint16_t i = 0; i < len; i++)
    // {
    //     Serial.printf("%x ", buf[i]);
    // }
    // Serial.println("");

    uint16_t slen = SCR_SERIAL.write(buf, len);
    return slen;
}

bool UART_SCR::sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len, bool checkFlag, int retry)
{
    memset(txbuff, 0, sizeof(txbuff));
    txbuff[0] = 0x5A;
    txbuff[1] = 0xA5;
    txbuff[2] = len + 3;
    txbuff[3] = cmd;
    memcpy(&txbuff[4], buf, len);
    uint16_t crc = calc_crc16(&txbuff[3], len + 1);
    txbuff[len + 4] = (uint8_t)(crc >> 8);
    txbuff[len + 5] = (uint8_t)crc;

    bool c = false;
    do
    {
        sendData(txbuff, len + 6);
        if (checkFlag)
        {
            c = check(CHECK_ACK);
        }
    } while (checkFlag && !c && (--retry > 0));

    if (retry <= 0)
    {
        Serial.println("[scr] wait ack timeout");
        return false;
    }

    return true;
}

uint16_t UART_SCR::readData(uint8_t *buf, uint16_t len, uint16_t timeOut)
{
    uint16_t rlen = 0;
    unsigned long time = millis();

    do
    {
        if (SCR_SERIAL.available())
        {
            rlen += SCR_SERIAL.read(&buf[rlen], len);
        }
    } while (rlen < len && (millis() - time < timeOut));

    // Serial.printf("read data size=%d\r\n", rlen);
    // for (uint16_t i = 0; i < len; i++)
    // {
    //     Serial.printf("%x ", rxbuff[i]);
    // }
    // Serial.println("");

    return rlen;
}

bool UART_SCR::checkReceiveFrame(uint8_t *buf, uint16_t len)
{
    /* 头校验 */
    uint16_t head = ((uint16_t)buf[0] << 8) | buf[1];
    if (head != FRAME_HEAD)
    {
        Serial.println("[scr] frame head unequal to 0x5AA5");
        return false;
    }

    /* 获取数据长度 */
    frameCheck.dataLen = buf[2] - 3;

    /* 校验crc */
    frameCheck.crc = ((uint16_t)buf[len - 2] << 8) | buf[len - 1];
    uint16_t crc = calc_crc16(&buf[3], frameCheck.dataLen + 1);
    if (crc != frameCheck.crc)
    {
        Serial.println("[scr] frame crc check failed");
        return false;
    }

    /* 获取数据 */
    frameCheck.frame = buf;
    frameCheck.frameLen = buf[2] + 3;
    frameCheck.cmd = buf[3];
    frameCheck.data = &buf[4];

    /* 获取按键值 */
    if (frameCheck.cmd == 0x83)
    {
        frameCheck.buttonAddr = ((uint16_t)frameCheck.data[0] << 8) + frameCheck.data[1];
        frameCheck.buttonVal = ((uint16_t)frameCheck.data[3] << 8) + frameCheck.data[4];
    }

    return true;
}

bool UART_SCR::check(CheckType_t type, uint16_t rlen)
{
    if (type == CHECK_NONE)
        return false;

    memset(rxbuff, 0, sizeof(rxbuff));

    /* 响应数据长度校验 */
    if (readData(rxbuff, rlen) != rlen)
    {
        return false;
    }

    /* 数据校验 */
    if (!checkReceiveFrame(rxbuff, rlen))
    {
        return false;
    }

    if (type == CHECK_ACK)
    {
        uint16_t data = ((uint16_t)frameCheck.data[0] << 8) | frameCheck.data[1];
        if (data != 0x4F4B)
        {
            Serial.println("[scr] ack check failed");
            return false;
        }
    }

    return true;
}

bool UART_SCR::writeFileToRam(File &file, uint16_t fileSize, uint16_t regAddr)
{
    uint8_t buf[242] = {0x00, 0x00};

    /* 将数据写入ram */
    while (fileSize > 0)
    {
        buf[0] = (uint8_t)(regAddr >> 8);
        buf[1] = (uint8_t)regAddr;

        size_t rd_len = file.read(&buf[2], ((fileSize > 240) ? 240 : fileSize));
        bool suc = sendFrame(0x82, buf, rd_len + 2);
        if (!suc)
        {
            Serial.println("[scr] write ram failed");
            return false;
        }
        if (fileSize > 0)
        {
            fileSize -= rd_len;
        }

        regAddr += rd_len / 2; // 一个地址可存储2字节数据，地址向后偏移120字节0x78
    }

    return true;
}

bool UART_SCR::saveToFlash(uint16_t flashAddr, uint16_t ramAddr)
{
    uint8_t buf[14] = {0x00, 0x00};

    /* 将数据转村到flash */
    buf[0] = 0x00;
    buf[1] = 0xAA;
    buf[2] = 0x5A;
    buf[3] = 0x02;
    buf[4] = (uint8_t)(flashAddr >> 8);
    buf[5] = (uint8_t)flashAddr;
    buf[6] = (uint8_t)(ramAddr >> 8);
    buf[7] = (uint8_t)ramAddr;

    return sendFrame(0x82, buf, 14);
}

bool UART_SCR::writeFile(const char *filePath, uint8_t saveID)
{
    if (saveID > 64)
    {
        Serial.println("[scr] font ID is not valid");
        return false;
    }

    File file = FFat.open(filePath);

    /* get file size */
    size_t size = file.size();

    uint16_t write_full_num = size / 32768;
    uint16_t write_left_size = size % 32768;

    uint16_t flashAddr = saveID * 256 / 32;

    Serial.printf("[scr] start write file: %s\r\n", &filePath[1]);
    /* 写32kb整包 */
    for (uint16_t i = 0; i < write_full_num; i++)
    {
        Serial.printf("[scr] writing page (%d)\r\n", i + 1);
        if (!writeFileToRam(file, 32768, SAVE_RAM_ADDR))
        {
            file.close();
            return false;
        }

        if (!saveToFlash(flashAddr++, SAVE_RAM_ADDR))
        {
            Serial.println("[scr] save to flash failed");
            file.close();
            return false;
        }
    }

    /* 写剩余包 */
    if (write_left_size > 0)
    {
        Serial.println("[scr] writing last page");
        if (!writeFileToRam(file, write_left_size, SAVE_RAM_ADDR))
        {
            file.close();
            return false;
        }

        if (!saveToFlash(flashAddr++, SAVE_RAM_ADDR))
        {
            Serial.println("[scr] save to flash failed");
            file.close();
            return false;
        }
    }

    Serial.printf("[scr] file write finish: %s \r\n", &filePath[1]);
    file.close();
    return true;
}

bool UART_SCR::readDataFormRam(uint16_t regAddr, int readLen)
{
    uint8_t buf[3] = {0x00, 0x00, 0x78};

    uint16_t j = 0;
    while (readLen > 0)
    {
        buf[0] = (uint8_t)(regAddr >> 8);
        buf[1] = (uint8_t)regAddr;

        readLen > 240 ? buf[2] = 0x78 : (buf[2] = (uint8_t)readLen / 2);

        uint16_t read_len = 9 + buf[2] * 2;
        Serial.printf("read_len = %d\r\n", read_len);

        bool c = false;
        int retry = 3;
        do
        {
            sendFrame(0x83, buf, 3, false); // 不校验ack
            c = check(CHECK_DATA, read_len);
        } while (!c && --retry > 0);
        if (retry <= 0)
        {
            Serial.println("scr read ram data check failed");
            return false;
        }

        Serial.printf("scr %d, read len=%d ", j++, read_len);
        Serial.printf("addr=0x%x, data = ", j++);
        for (uint8_t i = 0; i < read_len; i++)
        {
            Serial.printf("%x ", rxbuff[i]);
        }
        Serial.println("");

        regAddr += 0x78;

        if (readLen > 0)
        {
            readLen -= 240;
        }
    }

    if (SCR_SERIAL.available())
    {
        Serial.printf("no read all= %d \r\n", SCR_SERIAL.available());
        SCR_SERIAL.read(rxbuff, sizeof(rxbuff));
    }

    return true;
}

void UART_SCR::button_handle(uint16_t addr, uint16_t val)
{
    switch (addr)
    {
    case button_update:
        Serial.println("[scr] update button");
        // update_checkFile();
        this->upgrade();
        break;

    case 0x0002:
        Serial.println("TEST BUTTON");
        break;
    default:
        Serial.println("[scr] not define button");
        break;
    }
}

void UART_SCR::handle()
{
    static uint16_t rindex = 0;

    if (SCR_SERIAL.available())
    {
        rindex += readData(&rxbuff[rindex], SCR_SERIAL.available(), 0);
        return;
    }

    if (rindex == 0)
    {
        return;
    }

    // Serial.printf("read data size=%d\r\n", rindex);
    // for (uint16_t i = 0; i < rindex; i++)
    // {
    //     Serial.printf("%x ", rxbuff[i]);
    // }
    // Serial.println("");

    if (!checkReceiveFrame(rxbuff, rindex))
    {
        rindex = 0;
        return;
    }
    rindex = 0;

    button_handle(frameCheck.buttonAddr, frameCheck.buttonVal);
}

void UART_SCR::upgrade()
{
    Serial.println("[scr] Start update screen.");
    writeFile("/13TouchFile.bin", 13);
    writeFile("/14ShowFile.bin", 14);
    writeFile("/22_Config.bin", 22);
    writeFile("/32_Image.icl", 32);

    Serial.println("[scr] write update finish");
    delay(5000);
    reset();
}