#include "ymodem_host.h"

#define Y_C 0x43   // 大写字母Ｃ
#define Y_SOH 0x01 // 数据块起始字符 128字节
#define Y_STX 0x02 // 数据块起始字符 1024字节
#define Y_EOT 0x04 // 文件传输结束
#define Y_ACK 0x06 // 确认应答
#define Y_NAK 0x15 // 错误应答
#define Y_CAN 0x18 // 取消传输
#define Y_ALL 0xFF // 全校验
#define Y_SOH_LEN (128 + 5)
#define Y_STX_LEN (1024 + 5)

// static const char *y_str[] = {
//     "(0) file write success.",
//     "(1) file",
//     "(6) cancel transfer.",
// };

YmodemHost::YmodemHost(YmodemConfig_t *config) : ym_config(config)
{
    if (config != NULL)
    {
        /* dynamic memory */
        _dynamic = true;

        readBuff = new uint8_t[64]();
        sendBuff = new uint8_t[Y_STX_LEN]();
    }
    else
    {
        _dynamic = false;

        ym_config = NULL;
        sendBuff = NULL;
        readBuff = NULL;
    }

    _cancel = false;
}

YmodemHost::~YmodemHost()
{
    if (_dynamic)
    {
        ym_config = NULL;

        delete[] sendBuff;
        delete[] readBuff;
    }
}

bool YmodemHost::begin(YmodemConfig_t *config, uint8_t *ptx, uint8_t *prx)
{
    if (_dynamic)
    {
        return true;
    }

    if (config == NULL)
    {
        return false;
    }

    ym_config = config;
    sendBuff = ptx;
    readBuff = prx;

    return true;
}

void YmodemHost::end()
{
    ym_config = NULL;

    sendBuff = NULL;
    readBuff = NULL;
}

int YmodemHost::sendByte(uint8_t byte)
{
    uint8_t data[1] = {byte};

    return sendData(data, 1);
}

int YmodemHost::sendData(uint8_t *pbuf, uint16_t len)
{
    if (ym_config->sendData_callback == NULL)
    {
        return 0;
    }

    return ym_config->sendData_callback(pbuf, len, 0);
}

int YmodemHost::readData(uint8_t *pbuf, uint16_t len, uint16_t timeout)
{
    if (ym_config->readData_callback == NULL)
    {
        return 0;
    }

    int rlen = ym_config->readData_callback(pbuf, len, timeout);
    return rlen;
}

void YmodemHost::clear_cache()
{
    uint8_t buf[1];
    while (readData(buf, 1, 0))
        ;
}

void YmodemHost::get_log()
{
#ifdef YH_ERROR_LOG_EN

    memset(readBuff, 0, 64);
    int rlen = readData(readBuff, 64, 1000);

    if (rlen > 0)
    {
        Serial.printf("[ym log] %s", readBuff);
    }

    Serial.println("");
#endif
}

/**
 * @brief read ack data
 * @retval "-1" no data was read, "0" not valid data
 */
int YmodemHost::checkACK(uint8_t ack)
{
    int rlen = readData(readBuff, 1);

    if (rlen < 1)
    {
        return -1;
    }

    if (ack == Y_ALL)
    {
        switch (readBuff[0])
        {
        case Y_ACK:
            return Y_ACK;

        case Y_NAK:
            return Y_NAK;

        case Y_CAN:
            _cancel = true;
            return Y_CAN;

        default:
            Serial.printf("[ymodem] error ack:%x\r\n", readBuff[0]);
            break;
        }

        return 0;
    }

    if (readBuff[0] != ack)
    {
        Serial.printf("[ymodem] error ack:%x\r\n", readBuff[0]);
        return 0;
    }

    return readBuff[0];
}

bool YmodemHost::sendPackage(uint8_t pindex, uint8_t *buf, uint16_t len, int retry)
{
    if (len != Y_SOH_LEN && len != Y_STX_LEN)
    {
        Serial.println("[ymodem] sendPackage len error");
        return false;
    }

    /* ymodem head */
    len == Y_SOH_LEN ? buf[0] = Y_SOH : buf[0] = Y_STX;

    buf[1] = pindex;
    buf[2] = ~pindex;

    /* crc */
    uint16_t crc = ym_crc16(&buf[3], len - 5);
    buf[len - 2] = (uint8_t)(crc >> 8);
    buf[len - 1] = (uint8_t)crc;

#ifdef YH_ORIGINAL_EN
    retry = 0;
#endif

    /* send package and wait ack */
    int ack;
    do
    {
        sendData(buf, len);

        /* read ack */
#ifdef YH_ORIGINAL_EN
        ack = checkACK(Y_ACK);
#else
        ack = checkACK(Y_ALL);
#endif
        if (ack == Y_CAN)
            return false;

        /* not receive data */
        if (ack == -1)
            return false;

    } while ((ack != Y_ACK) && (--retry > 0));

    return ack == Y_ACK;
}

bool YmodemHost::start(const char *fileName, size_t size, int trynu)
{
    Serial.println("[ymodem] request a handshake");

    /* wait slave handshake */
    int ack;
    do
    {
        /* clear buffer */
        clear_cache();
        ack = checkACK(Y_C);
    } while ((ack != Y_C) && (--trynu > 0));
    if (ack != Y_C)
    {
        return false;
    }

    uint8_t *buf = sendBuff;

    /* ymodem file name */
    uint8_t name_len = strlen(fileName);
    memcpy(&buf[3], fileName, name_len);
    buf[name_len + 3] = 0x00;

    Serial.printf("[ymodem] file name: %s\r\n", fileName);

    /* ymodem file size */
    String str = String(size);
    uint32_t str_len = str.length();
    memcpy(&buf[name_len + 4], str.c_str(), str_len);
    buf[name_len + str_len + 4] = 0x00;

    Serial.printf("[ymodem] file size: %s\r\n", str.c_str());

    /* clear buffer */
    clear_cache();

    Serial.println("[ymodem] writing start package.");
    /* send package and wait ack */
    if (!sendPackage(0, buf, Y_SOH_LEN, 0))
    {
        Serial.println("[ymodem] first package send failed");
        return false;
    }

    /* wait slave ready */
    if (checkACK(Y_C) != Y_C)
    {
        return false;
    }

    return true;
}

bool YmodemHost::eot()
{
    sendByte(Y_EOT);

#ifdef YH_ORIGINAL_EN

    if (!checkACK(Y_NAK))
    {
        return false;
    }

    sendByte(Y_EOT);

    if (!checkACK(Y_ACK))
    {
        return false;
    }

    if (!checkACK(Y_C))
    {
        return false;
    }

    uint8_t *buf = sendBuff;
    memset(buf, 0, Y_STX_LEN);

    if (!sendPackage(0, buf, Y_SOH_LEN, 0))
    {
        return false;
    }

#else
    if (checkACK(Y_ACK) != Y_ACK)
    {
        return false;
    }

#endif

    return true;
}

bool YmodemHost::sendFile(fs::FS &fs, const char *filePath)
{
    if (ym_config == NULL)
    {
        return false;
    }

    /* open file */
    File file = fs.open(filePath);

    /* get file size */
    size_t size = file.size();

    /* send ymodem start package */
    if (!start(&filePath[1], size))
    {
        file.close();
        Serial.print("[ymodem] start failed: ");
        return false;
    }

    uint16_t full_num = size / 1024;
    uint16_t left_size = size % 1024;
    uint8_t *buf = sendBuff;

    /* send 1024byte package */
    Serial.printf("[ymodem] start write file: %s\r\n", &filePath[1]);
    for (uint16_t i = 0; i < full_num; i++)
    {
        Serial.printf("[ymodem] writing package (%d)\r\n", i + 1);

        size_t rd_len = file.read(&buf[3], 1024);

        if (!sendPackage(i + 1, buf, rd_len + 5))
        {
            file.close();
            Serial.printf("[ymodem] package(%d) write failed: ", i + 1);
            return false;
        }
    }

    /* send left package */
    if (left_size > 0)
    {
        uint16_t y_type;
        uint16_t fill_len;

        Serial.println("[ymodem] writing last package.");

        size_t rd_len = file.read(&buf[3], left_size);
        rd_len > 128 ? (y_type = Y_STX_LEN, fill_len = 1024 - rd_len) : (y_type = Y_SOH_LEN, fill_len = 128 - rd_len);
        memset(&buf[3 + rd_len], 0x1A, fill_len);

        if (!sendPackage(full_num + 1, buf, y_type))
        {
            file.close();
            Serial.print("[ymodem] last package write failed: ");
            return false;
        }
    }

    /* close file */
    file.close();

    /* send end flag */
    if (!eot())
    {
        Serial.println("[ymodem] end check failed.");
        return false;
    }

    Serial.printf("[ymodem] file \"%s\" write success.\r\n", &filePath[1]);
    return true;
}
