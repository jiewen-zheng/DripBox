#include "uartScreen.h"
#include "FFat.h"

#include "APP/common.h"

extern "C"
{
#include "Utils/src_crc16.h"
}

using namespace HAL;

HAL::UartScreen::UartScreen(MainBoard *mb)
{
    scrConfig = {
        .mask_type = 1,
        .milliliters = 2,
        .depth = 0,
        .runState = 0,
        .test = 0,
    };

    board = mb;
}

HAL::UartScreen::~UartScreen()
{
    delete board;
}

void HAL::UartScreen::setUpdateCallback(update_cb firm, update_cb soft)
{
    firmware_update = firm;
    software_update = soft;
}

void HAL::UartScreen::setWiFiCallback(save_wifi_cb wifi)
{
    save_wifi_msg = wifi;
}

void HAL::UartScreen::setVerMsg(VersionMsg_t *msg)
{
    versionMsg.dev = msg->dev;
    versionMsg.firm = msg->firm;
    versionMsg.soft = msg->soft;
}

void UartScreen::init()
{

    SCR_SERIAL.begin(115200, SERIAL_8N1, SCR_UART_RX_PIN, SCR_UART_TX_PIN);

    Block.init();
    // uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * 256);
    // if (!buf)
    // {
    //     log_e("scr uart tx buff malloc failed");
    //     return;
    // }

    // txbuff = buf;
}

void UartScreen::reset()
{
    uint8_t buf[6] = {0x00, 0x04, 0x55, 0xAA, 0x5A, 0XA5};

    Serial.println("[scr] reset");
    sendFrameNotCheck(0x82, buf, 6);
}

void HAL::UartScreen::clearCache()
{
    while (SCR_SERIAL.available())
    {
        SCR_SERIAL.read();
    }
}

uint16_t UartScreen::sendData(uint8_t *buf, uint16_t len)
{
    // debug
    // Serial.print("send data is:");
    // for (uint16_t i = 0; i < len; i++)
    // {
    //     Serial.printf("%x ", buf[i]);
    // }
    // Serial.println("");
    // debug

    uint16_t slen = SCR_SERIAL.write(buf, len);
    return slen;
}

bool UartScreen::sendFrame(uint8_t cmd, uint8_t *buf, uint16_t len, bool checkFlag, int retry)
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

void HAL::UartScreen::sendFrameNotCheck(uint8_t cmd, uint8_t *pbuf1, uint16_t len1, uint8_t *pbuf2, uint16_t len2)
{
    memset(txbuff, 0, sizeof(txbuff));
    txbuff[0] = 0x5A;
    txbuff[1] = 0xA5;
    txbuff[2] = len1 + len2 + 3;
    txbuff[3] = cmd;
    memcpy(&txbuff[4], pbuf1, len1);
    if (pbuf2 != nullptr && len2 != 0)
    {
        memcpy(&txbuff[4 + len1], pbuf2, len2);
    }

    uint16_t crc = calc_crc16(&txbuff[3], len1 + len2 + 1);
    txbuff[len1 + len2 + 4] = (uint8_t)(crc >> 8);
    txbuff[len1 + len2 + 5] = (uint8_t)crc;

    sendData(txbuff, len1 + len2 + 6);
}

uint16_t UartScreen::readData(uint8_t *buf, uint16_t len, uint16_t timeOut)
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

    // // debug
    // Serial.printf("read data size=%d\r\n", rlen);
    // for (uint16_t i = 0; i < len; i++)
    // {
    //     Serial.printf("%x ", rxbuff[i]);
    // }
    // Serial.println("");
    // // debug

    return rlen;
}

bool UartScreen::checkReceiveFrame(uint8_t *buf, uint16_t len)
{
    /* clear cache */
    memset(&frameCheck, 0, sizeof(FrameCheck_t));

    /* 头校验 */
    uint16_t head = buf[0] << 8 | buf[1];
    if (head != 0x5AA5)
    {
        Serial.println("[scr] frame head unequal to 0x5AA5");
        return false;
    }

    /* 获取数据长度 */
    uint16_t data_len = buf[2];
    frameCheck.dataLen = data_len - 3; // cmd + crc16

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
    frameCheck.frameLen = data_len + 3; // head + len
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

bool UartScreen::check(CheckType_t type, uint16_t rlen)
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

bool UartScreen::writeFileToRam(File &file, uint16_t fileSize, uint16_t regAddr)
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

bool UartScreen::saveToFlash(uint16_t flashAddr, uint16_t ramAddr)
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

bool UartScreen::writeFile(const char *filePath, uint8_t saveID)
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

bool UartScreen::readDataFormRam(uint16_t regAddr, int readLen)
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

void UartScreen::button_apply(uint16_t addr, uint16_t value)
{
    String read_msg = "";

    switch (addr)
    {
    case btn_baseStart:
        if (scrConfig.runState != 0 && scrConfig.runState != 1)
        {
            Serial.println("[scr] other runing");
            return;
        }

        if (!getUserSelectMsg())
        {
            return;
        }

        if (!board->baseLiquid(scrConfig.runState == 0, scrConfig.mask_type, scrConfig.milliliters, scrConfig.depth))
        {
            Serial.println("[scr] start failed");
            return;
        }

        scrConfig.runState = scrConfig.runState == 0 ? 1 : 0;

        updateIcon(icon_baseStart, scrConfig.runState != 0);
        break;

    case btn_dropStart:
        if (scrConfig.runState != 0 && scrConfig.runState != 2)
        {
            Serial.println("[scr] other runing");
            return;
        }

        if (!getUserSelectMsg())
        {
            return;
        }

        if (!board->dropLiquid(scrConfig.runState == 0, scrConfig.mask_type, scrConfig.milliliters, scrConfig.depth))
        {
            Serial.println("[scr] start failed");
            return;
        }

        scrConfig.runState = scrConfig.runState == 0 ? 2 : 0;

        updateIcon(icon_dropStart, scrConfig.runState != 0);
        break;

    case btn_emptying:
        if (scrConfig.runState != 0 && scrConfig.runState != 3)
        {
            Serial.println("[scr] other runing");
            return;
        }
        // scrConfig.runState = scrConfig.runState == 0 ? 3 : 0;
        scrConfig.runState != 0 ? pushTheButton(popup_empty_stop) : pushTheButton(popup_empty_start);

        break;

    case btn_popup_empty_start:
        if (value != 0)
        {
            if (!board->empty(scrConfig.runState == 0))
            {
                Serial.println("[scr] start failed");
                return;
            }
            scrConfig.runState = scrConfig.runState == 0 ? 3 : 0;

            updateIcon(icon_emptying, scrConfig.runState != 0);
        }
        break;

    case btn_popup_empty_stop:
        if (value != 0)
        {
            if (!board->empty(scrConfig.runState == 0))
            {
                Serial.println("[scr] stop failed");
                return;
            }

            scrConfig.runState = scrConfig.runState == 0 ? 3 : 0;
            updateIcon(icon_emptying, scrConfig.runState != 0);
        }
        break;

    case btn_depthSelect:
        if (scrConfig.runState == 0)
        {
            updateIcon(icon_depthSelect, value);
        }
        break;

    case btn_milliliter:
        if (scrConfig.runState == 0)
        {
            pushTheButton(popup_milliliter);
        }
        break;

    case btn_maskSelect:
        if (scrConfig.runState == 0)
        {
            updateIcon(icon_maskSelect, value);
        }
        break;

    case btn_wifi_page:
        if (scrConfig.runState == 0)
        {
            pageSwitch(1);
        }
        break;

    case btn_calibrat:
        if (scrConfig.runState == 0)
        {
            pageSwitch(7);
        }
        break;

    case btn_connectWiFi:
        if (connectWiFi(getWifiSSID(), getWifiPASS()))
        {
            save_wifi_msg(wifi_ssid.c_str(), wifi_pass.c_str());
        }
        break;

    case btn_firmwareUpdate:
        firmware_update();
        break;

    case btn_softwareUpdate:
        software_update();
        break;

    case btn_testRadeWrite:
        if (value == 1)
        {
            /* read */
            board->readOffset();

            /* msg update in "syncDevice()" */
            // if (board->readOffset())
            // {
            //     writeAddrData(text_testRade, (uint8_t *)board->getOffsetMsg().c_str(), board->getOffsetMsg().length());
            // }
        }
        else if (value == 2)
        {
            /* write */
            if (board->writeOffset())
            {
                writeAddrData(text_testWrite, (uint8_t *)board->getOffsetMsg().c_str(), board->getOffsetMsg().length());
            }
        }
        break;

    case btn_testPoint:
        if (board->moveTestPoint(value))
        {
            updateTestPointIcon(value);
        }
        break;

    case btn_testMove:
        board->setOffset(value);
        break;

    case btn_testStart:
        scrConfig.test ^= 0x01;
        board->moveTest(scrConfig.test);
        break;

    case btn_format:
        if (checkFormatPass(&frameCheck.data[3]))
        {
            Serial.println("[scr] format");
            FFat.end();
            FFat.format();
            reset();
            ESP.restart();
        }
        break;
    default:
        Serial.println("[scr] not define button");
        break;
    }
}

void HAL::UartScreen::dataPack_handle()
{
    static uint16_t rindex = 0;
    /* get uart data */
    if (SCR_SERIAL.available())
    {
        rindex += readData(&rxbuff[rindex], SCR_SERIAL.available(), 0);
        return;
    }
    if (rindex == 0)
    {
        return;
    }

    /* check screen frame */
    if (!checkReceiveFrame(rxbuff, rindex))
    {
        rindex = 0;
        return;
    }
    rindex = 0;

    /* handle button */
    button_apply(frameCheck.buttonAddr, frameCheck.buttonVal);
}

void HAL::UartScreen::syncDevice(uint16_t time)
{
    static unsigned long inter_time = millis();

    if (millis() - inter_time < time)
    {
        return;
    }
    inter_time = millis();

    /* update the icon when the run stops */
    updateRunState();

    updateOffsetMsg();
}

void UartScreen::handle()
{
    /* update wifi rssi icon */
    updateRSSI();

    /* sync device all message */
    syncDevice();

    /* uart frame data handle */
    dataPack_handle();
}

void UartScreen::upgrade()
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

bool HAL::UartScreen::upgreadFile(const char *path, int fileNumber)
{
    Serial.printf("upgreadFilePath: %s \r\n", path);
    Serial.printf("number: %d \r\n", fileNumber);
    // return false;
    return writeFile(path, fileNumber);
}

uint8_t *HAL::UartScreen::getAddrData(uint16_t addr, uint16_t len)
{
    uint8_t buff[3];

    buff[0] = addr >> 8;
    buff[1] = addr;
    buff[2] = len;
    sendFrameNotCheck(0x83, buff, 3);

    /* 帧头2 + 长度1 + 指令1 + 地址2 + 数据长度1 + crc2*/
    uint16_t data_len = len * 2 + 9;
    memset(rxbuff, 0, sizeof(rxbuff));

    uint16_t r_len = readData(rxbuff, data_len);

    if (r_len < data_len)
    {
        return nullptr;
    }

    if (!checkReceiveFrame(rxbuff, data_len))
    {
        return nullptr;
    }

    return frameCheck.data + 3;
}

int HAL::UartScreen::getAddrData(uint16_t addr)
{
    /* 一个数据等于两个字节 */
    uint8_t *pdata = getAddrData(addr, 1);

    if (pdata == nullptr)
    {
        return -1;
    }

    int data = (pdata[0] << 8) | pdata[1];
    return data;
}

void HAL::UartScreen::writeAddrData(uint16_t addr, uint8_t *pdata, uint16_t len)
{
    uint8_t clear[36] = {0};

    uint8_t buff[2];
    buff[0] = addr >> 8;
    buff[1] = addr;

    clear[0] = addr >> 8;
    clear[1] = addr;

    /* clear */
    sendFrame(0x82, clear, 32);

    sendFrameNotCheck(0x82, buff, 2, pdata, len);
}

bool HAL::UartScreen::getUserSelectMsg()
{
    int readData = -1;

    // 获取面膜类型
    readData = getAddrData(icon_maskSelect);
    if (readData != -1)
    {
        Serial.printf("maks = %d", readData);
        scrConfig.mask_type = readData;
    }
    else
    {
        return false;
    }

    // 获取深浅选择
    readData = getAddrData(icon_depthSelect);
    if (readData != -1)
    {
        Serial.printf("depth = %d", readData);
        scrConfig.depth = readData;
    }
    else
    {
        return false;
    }

    // 获取毫升数
    readData = getAddrData(icon_milliliter);
    if (readData != -1)
    {
        Serial.printf("depth = %d", readData);
        scrConfig.milliliters = readData;
    }
    else
    {
        return false;
    }

    return true;
}

void HAL::UartScreen::updateIcon(uint16_t addr, uint16_t value)
{
    uint8_t buff[4];

    buff[0] = addr >> 8;
    buff[1] = addr;
    buff[2] = value >> 8;
    buff[3] = value;

    /* check feedback "OK" */
    sendFrame(0x82, buff, 4);
    // sendFrameNotCheck(0x82, buff, 4);
}

void HAL::UartScreen::updateTestPointIcon(uint8_t value)
{
    updateIcon(icon_testLeftEye, 0);
    updateIcon(icon_testRightEye, 0);
    updateIcon(icon_testMouth, 0);
    if (value == 1)
    {
        updateIcon(icon_testLeftEye, 1);
    }
    else if (value == 2)
    {
        updateIcon(icon_testRightEye, 1);
    }
    else if (value == 3)
    {
        updateIcon(icon_testMouth, 1);
    }
}

void HAL::UartScreen::pushTheButton(uint8_t pushNu)
{
    uint8_t buff[10];

    buff[0] = 0x00;
    buff[1] = 0xd4;
    buff[2] = 0x5a;
    buff[3] = 0xa5;
    buff[4] = 0x00;
    buff[5] = 0x04;
    buff[6] = 0xff;
    buff[7] = pushNu;
    buff[8] = 0x00;
    buff[9] = 0x01;

    /* check feedback "OK" */
    sendFrame(0x82, buff, 10);
}

void HAL::UartScreen::dispVersion(const char *device_id, const char *firmware, const char *software)
{
    writeAddrData(text_deviceID, (uint8_t *)device_id, strlen(device_id));

    writeAddrData(text_firmwareVer, (uint8_t *)firmware, strlen(firmware));
    writeAddrData(text_softwareVer, (uint8_t *)software, strlen(software));
}

void HAL::UartScreen::LogUpgradeMsg(const char *msg)
{
    writeAddrData(text_updateLog, (uint8_t *)msg, strlen(msg));
}

void HAL::UartScreen::updateVerMsg()
{
    dispVersion(versionMsg.dev.c_str(), versionMsg.firm.c_str(), versionMsg.soft.c_str());
}

void HAL::UartScreen::updateRunState()
{
    if (scrConfig.runState == 0)
    {
        Block.unlock();
        return;
    }

    /* lock box */
    Block.lock();

    BoardRunState_t board_state = board->getDeviceRunState();

    // Serial.printf("board state%d", board_state);

    switch (board_state)
    {
    case STOP_ZERO: // basic liquid
    case STOP:
        updateIcon(icon_baseStart, 0);
        updateIcon(icon_dropStart, 0);
        updateIcon(icon_emptying, 0);
        scrConfig.runState = 0;
        break;

    case GOTO_ZERO:
        break;

    case EMPTY:
        updateIcon(icon_emptying, 1);
        scrConfig.runState = 3;
        break;

    case BASIC:
        updateIcon(icon_baseStart, 1);
        scrConfig.runState = 1;
        break;

    case DROP:
        updateIcon(icon_dropStart, 1);
        scrConfig.runState = 2;
        break;

    default:
        break;
    }
}

void HAL::UartScreen::updateOffsetMsg()
{
    String msg = board->getOffsetMsg();

    if (msg.length() == 0)
    {
        return;
    }

    writeAddrData(text_testRade, (uint8_t *)msg.c_str(), msg.length());
}

bool HAL::UartScreen::checkFormatPass(uint8_t *pass)
{
    if (memcmp(pass, FORMAT_PASS, 6) == 0)
    {
        return true;
    }

    return false;
}

String HAL::UartScreen::getWifiSSID()
{
    char data[32] = {0};

    /* get wifi ssid */
    uint8_t *ssid = getAddrData(text_wifiSSID, 16);
    if (!ssid)
    {
        return "";
    }
    char *sptr = strchr((char *)ssid, 0xff);
    if (!sptr)
    {
        return "";
    }
    memcpy(data, ssid, sptr - (char *)ssid);

    wifi_ssid = String(data);
    Serial.printf("ssid = ");
    Serial.println(wifi_ssid);
    return wifi_ssid;
}

String HAL::UartScreen::getWifiPASS()
{
    char data[32] = {0};

    /* get wifi pass */
    uint8_t *pass = getAddrData(text_wifiPASS, 16);
    if (!pass)
    {
        Serial.println("pass get faile");
        return "";
    }
    char *pptr = strchr((char *)pass, 0xff);
    if (!pptr)
    {
        Serial.println("pass extract error");
        return "";
    }
    memcpy(data, pass, pptr - (char *)pass);

    wifi_pass = String(data);
    Serial.printf("pass = ");
    Serial.println(wifi_pass);
    return wifi_pass;
}

void HAL::UartScreen::updateRSSI(uint16_t update_time)
{
    static unsigned long time = millis();

    if (millis() - time < update_time)
    {
        return;
    }
    time = millis();

    int8_t rssi = getWifiRSSI();

    if (rssi == 0)
    {
        updateIcon(icon_wifi, 0);
        return;
    }

    uint8_t value = (rssi / 30) + 4;

    if (value < 1)
        value = 1;
    else if (value > 3)
        value = 3;

    updateIcon(icon_wifi, value);
}

void HAL::UartScreen::pageSwitch(uint16_t page)
{
    uint8_t buf[6] = {0};

    /* 将数据转村到flash */
    buf[0] = 0x00;
    buf[1] = 0x84;
    buf[2] = 0x5A;
    buf[3] = 0x01;
    buf[4] = page >> 8;
    buf[5] = page;

    sendFrame(0x82, buf, 6);
}

void HAL::UartScreen::firmware_have_update()
{
    updateIcon(icon_firmwareUpdate, 0x0001);
}

void HAL::UartScreen::software_have_update()
{
    updateIcon(icon_softwareUpdate, 0x0001);
}
