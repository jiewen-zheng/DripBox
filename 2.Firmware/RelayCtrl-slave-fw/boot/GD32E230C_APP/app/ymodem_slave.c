#include "ymodem_slave.h"
#include "systick.h"
#include <string.h>
#include <stdlib.h>

#define Y_SOH 0x01          // package head, data length use 128 byte
#define Y_STX 0x02          // package head, data length use 1024 byte
#define Y_EOT 0x04          // end of transfer
#define Y_ACK 0x06          // ack
#define Y_NAK 0x15          // transfer error ack
#define Y_CAN 0x18          // cancel transfer
#define Y_C 0x43            // 'C' ascii val
#define Y_SOH_LEN (128 + 5) // 3 byte package head, 2 byte crc.
#define Y_STX_LEN (1024 + 5)

static const char *yslave_str[] = {
    "(0) update succeed.",
    "(1) unknown error.",
    "(2) wait timeout.",
    "(3) transfer data check error.",
    "(4) file size is too long.",
    "(5) end check error, But it doesn't affect the result.",
    "(6) cancel update.",
};
/* save iap error message */
static Yslave_status_t error = ys_unknow;

/* file infomation */
static FileInfo_t *file;

/* file package cache */
static uint8_t *packageBuff;

/* ymodem callback function */
static Yslave_config_t *ys_config;

/* ymodem crc table buffer */
static const uint16_t crc_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

static uint16_t ys_calc_crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc16 = 0x0000;
    uint16_t crc_h8, crc_l8;

    while (len--)
    {
        crc_h8 = (crc16 >> 8);
        crc_l8 = (crc16 << 8);
        crc16 = crc_l8 ^ crc_table[crc_h8 ^ *data];
        data++;
    }

    return crc16;
}

bool yslave_init(Yslave_config_t *config)
{
    if (config == NULL)
        return false;

    FileInfo_t *fi = (FileInfo_t *)malloc(sizeof(FileInfo_t));
    if (!fi)
    {
        return false;
    }

    uint8_t *pack = (uint8_t *)malloc(sizeof(uint8_t) * Y_STX_LEN);
    if (!pack)
    {
        free(fi);
        return false;
    }

    file = fi;
    packageBuff = pack;
    ys_config = config;

    return true;
}

void ys_send_data(uint8_t *pbuf, uint16_t len)
{
    if (ys_config->sendData_callback == NULL)
        return;

    Yslave_transfer_t trans = {
        .pbuf = pbuf,
        .len = len,
        .timeout = 0,
    };

    ys_config->sendData_callback(&trans);
}

void ys_send_byte(uint8_t byte)
{
    uint8_t data[1] = {byte};

    ys_send_data(data, 1);
}

bool ys_read_package(uint8_t *pbuf, uint16_t len, uint16_t timeout)
{
    if (ys_config->readData_callback == NULL)
    {
        return false;
    }

    Yslave_transfer_t trans = {
        .pbuf = pbuf,
        .len = len,
        .timeout = timeout,
    };

    int rlen = ys_config->readData_callback(&trans);

    return len == rlen;
}

void ys_clear_cache()
{
    uint8_t buf[1];

    while (ys_read_package(buf, 1, 0))
        ;
}

bool ys_store(uint32_t addr, uint32_t *data, uint32_t size)
{
    if (ys_config->store_callback == NULL)
    {
        return false;
    }

    Yslave_store_t store = {
        .store_addr = addr,
        .store_data = data,
        .store_size = size,
    };

    int es = ys_config->store_callback(&store);

    if (es == ys_storeError)
    {
        error = ys_storeError;
        return false;
    }

    return true;
}

static int getstrpos(char *str, char *substr)
{
    char *s = strstr(str, substr);
    if (!s)
    {
        return -1;
    }

    size_t len1 = strlen(str);
    size_t len2 = strlen(s);

    return len1 - len2;
}

bool checkCRC(uint8_t *buff, uint32_t size)
{
    uint8_t *p = buff + size - 5;
    uint16_t crc_res = 0;

    crc_res = ys_calc_crc16(buff, size - 5);
    return ((((uint8_t)((crc_res & 0xFF00) >> 8) == *p) && ((uint8_t)(crc_res & 0x00FF) == *(p + 1))) ? 1 : 0);
}

static bool checkACK(uint8_t ack)
{
    uint8_t rbuf[1];

    if (!ys_read_package(rbuf, 1, YS_WAIT_TIME))
    {
        return false;
    }

    return rbuf[0] == ack;
}

bool check_first_package()
{
    uint8_t *rbuf = packageBuff;
    /* 检查接收数据长度 */
    if (!ys_read_package(rbuf, Y_SOH_LEN, YS_WAIT_TIME))
    {
        error = ys_timeout;
        return false;
    }

    if (rbuf[0] != Y_SOH || rbuf[1] != 0x00 || rbuf[2] != 0xFF)
    {
        error = ys_transferError;
        return false;
    }

    /* 将数据对齐到buff头部，避免弟弟单片机内核无法将8位指针偏移转32位指针 */
    memcpy(rbuf, rbuf + 3, Y_SOH_LEN - 3);

    if (!checkCRC(rbuf, Y_SOH_LEN))
    {
        error = ys_transferError;
        return false;
    }

    int pos = getstrpos((char *)&rbuf[3], (char *)".bin");
    if (pos == -1)
    {
        error = ys_transferError;
        return false;
    }

    memset(file->Name, 0, YS_FILENAME_MAX_LEN);
    memcpy(file->Name, &rbuf[3], pos + 4);

    char *ptr;
    /* 文件名后有1空格字符，帧头占用3字符，文件后缀.bin占用4字符，所以文件大小位置偏移8字节 */
    file->Size = strtoul((char *)&rbuf[pos + 8], &ptr, 10);
    if (file->Size > ys_config->maxFilesize)
    {
        /* log error message */
        error = ys_fileToolong;
        return false;
    }

    return true;
}

bool check_mid_package(uint16_t index)
{
    uint8_t *rbuf = packageBuff;
    if (!ys_read_package(rbuf, Y_STX_LEN, YS_WAIT_TIME))
    {
        error = ys_timeout;
        return false;
    }

    if (rbuf[0] != Y_STX || rbuf[1] != index || rbuf[2] != (uint8_t)(~index))
    {
        error = ys_transferError;
        return false;
    }
    /* 将数据对齐到buff头部，避免弟弟单片机内核无法将8位指针偏移转32位指针 */
    memcpy(rbuf, rbuf + 3, Y_STX_LEN - 3);

    return checkCRC(rbuf, Y_STX_LEN);
}

bool check_end_package(uint8_t index, uint16_t size)
{
    uint16_t read_len;
    uint8_t head;

    /* make sure last package length */
    size <= 128 ? (read_len = Y_SOH_LEN, head = Y_SOH) : (read_len = Y_STX_LEN, head = Y_STX);

    uint8_t *rbuf = packageBuff;
    if (!ys_read_package(rbuf, read_len, YS_WAIT_TIME))
    {
        /* not receive fixed length data. */
        error = ys_timeout;
        return false;
    }

    /* check package head */
    if (rbuf[0] != head || rbuf[1] != index || rbuf[2] != (uint8_t)(~index))
    {
        error = ys_transferError;
        return false;
    }
    /* 将数据对齐到buff头部，避免弟弟单片机内核无法将8位指针偏移转32位指针 */
    memcpy(rbuf, rbuf + 3, read_len - 3);

    return checkCRC(rbuf, read_len);
}

#ifdef YS_ORIGINAL_EN
bool check_eot_package()
{
    uint8_t *rbuf = packageBuff;
    if (!ys_read_package(rbuf, Y_SOH_LEN, YS_WAIT_TIME))
    {
        /* not receive fixed length data. */
        error = ys_timeout;
        return false;
    }

    /* check package head */
    if (rbuf[0] != Y_SOH || rbuf[1] != 0 || rbuf[2] != 0xff)
    {
        error = ys_transferError;
        return false;
    }

    /* 将数据对齐到buff头部，避免弟弟单片机内核无法将8位指针偏移转32位指针 */
    memcpy(rbuf, rbuf + 3, Y_SOH_LEN - 3);

    return checkCRC(rbuf, Y_SOH_LEN);
}
#endif

bool start()
{
    ys_send_byte(Y_C);

    if (!check_first_package())
    {
        ys_clear_cache();
        return false;
    }

    /* start package receive ack */
    ys_send_byte(Y_ACK);

    return true;
}

bool run()
{
    uint16_t package_index = 0;
    int retry_num = YS_RETRY_NUM;

    uint16_t full_package = (uint16_t)(file->Size >> 10);
    uint16_t left_package_size = (uint16_t)(file->Size - (full_package * 1024));

    /* ready receive package */
    ys_send_byte(Y_C);

    while (package_index < full_package)
    {
        if (!check_mid_package(package_index + 1))
        {

#ifdef YS_ORIGINAL_EN
            return false;
#else
            if (--retry_num < 0)
            {
                break;
            }
            ys_clear_cache();
            ys_send_byte(Y_NAK);
            continue;
#endif
        }

        /* store data */
        if (!ys_store(ys_config->storeStartAddr + package_index * 1024,
                      (uint32_t *)packageBuff, 1024))
        {
            return false;
        }

        // program_flash(USER_APP_START_ADDR + package_index * 1024, (uint32_t *)packageBuff, 1024); // 跳过ymodem帧头

        /* 处理完成正确应答 */
        ys_send_byte(Y_ACK);
        /* 处理下一包 */
        package_index += 1;

        retry_num = YS_RETRY_NUM;
    }

    /* 包接收不完整判断 */
    if (package_index < full_package)
    {
        return false;
    }
		
		/* 无剩余数据 */
		if(left_package_size == 0){
			return true;
		}

			
    /* 处理剩余数据 */
		retry_num = YS_RETRY_NUM;
    while (--retry_num > 0)
    {
        if (!check_end_package(package_index + 1, left_package_size))
        {
#ifdef YS_ORIGINAL_EN
            return false;
#else
            ys_send_byte(Y_NAK);
            continue;
#endif
        }

        memset(&packageBuff[left_package_size], 0xFF, Y_STX_LEN - left_package_size);

        /* store file data */
        if (!ys_store(ys_config->storeStartAddr + package_index * 1024,
                      (uint32_t *)packageBuff, 1024))
        {
            return false;
        }

        // program_flash(USER_APP_START_ADDR + package_index * 1024, (uint32_t *)packageBuff, 1024);

        ys_send_byte(Y_ACK);
        break;
    }

    if (retry_num < 0)
    {
        return false;
    }

    return true;
}

bool end()
{
    if (!checkACK(Y_EOT))
    {
        return false;
    }

#ifdef YS_ORIGINAL_EN
    ys_send_byte(Y_NAK);

    if (!checkACK(Y_EOT))
    {
        return false;
    }

    ys_send_byte(Y_ACK);

    ys_send_byte(Y_C);

    if (!check_eot_package())
    {
        return false;
    }

#endif

    ys_send_byte(Y_ACK);

    error = ys_succeed;

    /* free the memory */
    ys_config = NULL;
    free(packageBuff);
    packageBuff = NULL;

    return true;
}

Yslave_status_t yslave_handle()
{
    if (error == ys_succeed)
    {
        return ys_succeed;
    }

    if (!start())
    {
#ifdef YS_LOG_EN
        ys_send_data((uint8_t *)yslave_str[error], strlen(yslave_str[error]));
#endif
        return error;
    }

    if (!run())
    {
#ifdef YS_LOG_EN
        ys_send_data((uint8_t *)yslave_str[error], strlen(yslave_str[error]));
#endif
        return error;
    }

    if (!end())
    {
#ifdef YS_LOG_EN
        ys_send_data((uint8_t *)yslave_str[error], strlen(yslave_str[error]));
#endif
        return error;
    }

    return ys_succeed;
}

FileInfo_t *yslave_getFileInfo()
{
    return file;
}
