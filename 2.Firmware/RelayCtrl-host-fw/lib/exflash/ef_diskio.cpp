

#include "ef_diskio.h"

extern "C"
{
#include "ff.h"
#include "diskio.h"
#if ESP_IDF_VERSION_MAJOR > 3
#include "diskio_impl.h"
#endif
// #include "esp_vfs.h"
#include "esp_vfs_fat.h"
}

typedef struct
{
    uint8_t ssPin;
    SPIClass *spi;
    SPISettings _settings;
    int frequency;
    char *base_path;
    uint32_t sectorCount;
    uint16_t sectorSize;
    uint32_t bolckSize;
    uint16_t deviceID;
    uint16_t jedecID;
    int status;
} ardu_exflash_t;

static ardu_exflash_t *e_flashs[FF_VOLUMES] = {NULL};
static uint8_t *e_readCache;

/**
 * FLASH SPI
 */

bool efUnselect(uint8_t pdrv)
{
    digitalWrite(e_flashs[pdrv]->ssPin, HIGH);
    e_flashs[pdrv]->spi->endTransaction();

    return true;
}

bool efSelect(uint8_t pdrv)
{
    e_flashs[pdrv]->spi->beginTransaction(e_flashs[pdrv]->_settings);

    digitalWrite(e_flashs[pdrv]->ssPin, LOW);

    return true;
}

/**
 * @brief read flash status.
 * @param pdrv drive number.
 * @return flash now status.
 */
uint8_t efReadStatus(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efSelect(pdrv);
    ef->spi->transfer(W25QX_ReadStatusReg);
    uint8_t status = ef->spi->transfer(0);

    efUnselect(pdrv);
    log_i("exflash read status = %x", status);

    return status;
}

bool efBusy(uint8_t pdrv, uint32_t timeout = 5000)
{
    uint32_t start = millis();
    bool resp;
    do
    {
        resp = efReadStatus(pdrv) & 0x01;
    } while (resp == true && (millis() - start) < timeout);

    if (resp)
    {
        log_w("exflash busy");
    }
    return resp;

    // return efReadStatus(pdrv) & 1;
}

/**
 * @brief use spi send command to flash.
 * @param pdrv drive number.
 * @param cmd send command.
 * @param isWrite flash write enable
 *                true is send write enable cmd, false is send user cmd
 * @return None.
 */
void efCommand(uint8_t pdrv, uint8_t cmd, bool isWrite = false)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    if (isWrite)
    {
        efCommand(pdrv, W25QX_WriteEnable);
        efUnselect(pdrv);
    }

    if (cmd != W25QX_ReleasePowerDown)
    {
        while (efBusy(pdrv))
        {
        }
    }

    efSelect(pdrv);
    ef->spi->transfer(cmd);
}

/**
 * @brief read Manufacturer and Device Identification
 * @param pdrv drive number.
 * @retval device manufacturer id.
 */
uint16_t efReadDeviceID(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efSelect(pdrv);
    ef->spi->transfer(W25QX_ReadManufactDeviceID);
    ef->spi->transfer(0);
    ef->spi->transfer(0);
    ef->spi->transfer(0);

    uint16_t deviceid = ef->spi->transfer(0) << 8;
    deviceid |= ef->spi->transfer(0);
    efUnselect(pdrv);

    log_d("w25qx device id = %x", deviceid);
    return deviceid;
}

/**
 * @brief read Jedec Identification
 * @param pdrv drive number.
 * @retval device jedec id.
 */
uint16_t efReadJedecID(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efSelect(pdrv);
    ef->spi->transfer(W25QX_ReadJedecID);

    uint16_t jedecid = ef->spi->transfer(0) << 8;
    jedecid |= ef->spi->transfer(0);
    efUnselect(pdrv);

    log_d("w25qx jedecid = %x", jedecid);
    return jedecid;
}

/**
 * @brief read one byte form flash
 * @param pdrv drive number
 * @param addr read address
 * @retval one byte data
 */
uint8_t efReadByte(uint8_t pdrv, uint32_t addr)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efCommand(pdrv, W25QX_ReadData);
    ef->spi->transfer(addr >> 16);
    ef->spi->transfer(addr >> 8);
    ef->spi->transfer(addr);
    uint8_t result = ef->spi->transfer(0);
    efUnselect(pdrv);
    return result;
}

/**
 * @brief read a piece of data to cache form flash
 * @param pdrv drive number
 * @param addr read address
 * @param buf cache pointer
 * @param len read data len, onece max read 65535 byte.
 * @return None
 */
void efReadData(uint8_t pdrv, uint32_t addr, void *buf, uint16_t len)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efCommand(pdrv, W25QX_FastRead);
    ef->spi->transfer(addr >> 16);
    ef->spi->transfer(addr >> 8);
    ef->spi->transfer(addr);
    ef->spi->transfer(0); //"fast read require add a dummy clocks"
    for (uint16_t i = 0; i < len; ++i)
    {
        ((uint8_t *)buf)[i] = ef->spi->transfer(0);
    }
    efUnselect(pdrv);
}

void efSectorErase4K(uint8_t pdrv, uint32_t addr)
{
    ardu_exflash_t *ef = e_flashs[pdrv];
    efCommand(pdrv, W25QX_SectorErase_4KB, true);
    ef->spi->transfer(addr >> 16);
    ef->spi->transfer(addr >> 8);
    ef->spi->transfer(addr);
    efUnselect(pdrv);
}

void efSectorErase32K(uint8_t pdrv, uint32_t addr)
{
    ardu_exflash_t *ef = e_flashs[pdrv];
    efCommand(pdrv, W25QX_SectorErase_32KB, true);
    ef->spi->transfer(addr >> 16);
    ef->spi->transfer(addr >> 8);
    ef->spi->transfer(addr);
    efUnselect(pdrv);
}

void efSectorErase64K(uint8_t pdrv, uint32_t addr)
{
    ardu_exflash_t *ef = e_flashs[pdrv];
    efCommand(pdrv, W25QX_SectorErase_64KB, true);
    ef->spi->transfer(addr >> 16);
    ef->spi->transfer(addr >> 8);
    ef->spi->transfer(addr);
    efUnselect(pdrv);
}

/**
 * @brief write bytes to flash
 * @param pdrv drive number
 * @param addr write address
 * @param buf  write data pointer
 * @param len  write data length
 * @return None
 * @warning you can only write to previously erased memory locations (see datasheet)
 *          use the block erase commands to first clear memory (write 0xFFs)
 */
void efWriteBytes(uint8_t pdrv, uint32_t addr, const void *buf, uint16_t len)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    uint16_t n;
    uint16_t maxBytes = 256 - (addr % 256);
    uint16_t offset = 0;
    while (len > 0)
    {
        n = (len <= maxBytes) ? len : maxBytes;
        efCommand(pdrv, W25QX_PageProgram, true);
        ef->spi->transfer(addr >> 16);
        ef->spi->transfer(addr >> 8);
        ef->spi->transfer(addr);
        for (uint16_t i = 0; i < n; i++)
        {
            ef->spi->transfer(((uint8_t *)buf)[offset + i]);
        }
        efUnselect(pdrv);

        addr += n;
        offset += n;
        len -= n;
        maxBytes = 256;
    }
}

void efWriteData(uint8_t pdrv, uint32_t addr, const void *buf, uint16_t len)
{
    uint8_t *read_p = e_readCache;

    ardu_exflash_t *ef = e_flashs[pdrv];

    uint16_t n;
    uint32_t sector_baseAddr = (addr / 4096) * 4096; // 扇区基地址
    uint16_t sector_offset = addr % 4096;
    uint16_t sector_remain = 4096 - sector_offset; // 扇区剩余空间大小
    uint16_t w_offset = 0;

    while (len > 0)
    {
        n = (len <= sector_remain) ? len : sector_remain;

        /* 读出扇区数据 */
        efReadData(pdrv, sector_baseAddr, read_p, 4096);
        uint16_t i;
        for (i = 0; i < n; i++)
        {
            if (read_p[i] != 0xff)
            {
                efSectorErase4K(pdrv, sector_baseAddr);
                memcpy(&read_p[sector_offset], &((uint8_t *)buf)[w_offset], sector_remain);
                efWriteBytes(pdrv, sector_baseAddr, read_p, 4096);
                break;
            }
        }

        if (i >= n)
        {
            efWriteBytes(pdrv, addr + w_offset, (uint8_t *)buf + w_offset, n);
        }

        sector_baseAddr += 4096;
        sector_offset = 0;
        sector_remain = 4096;

        w_offset += n;
        len -= n;
    }
}

/**
 * @brief erase entire flash memory array.
 * @param pdrv drive number.
 * @warning may take several seconds depending on size, but is non blocking,
 *          so you may wait for this to complete using efBusy() or continue doing,
 *          other things and later check if the chip is done with efBusy(),
 *          note that any command will first wait for chip to become available using efBusy(),
 *          so no need to do that twice.
 */
void efChipErase(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];

    efCommand(pdrv, W25QX_ChipErase, true);
    efUnselect(pdrv);
}

void efSleep(uint8_t pdrv)
{
    efCommand(pdrv, W25QX_PowerDown);
    efUnselect(pdrv);
}

void efWakeup(uint8_t pdrv)
{
    efCommand(pdrv, W25QX_ReleasePowerDown);
    efUnselect(pdrv);
}

/**
 * SPI FLASH Communication
 */

/**
 * FATFS API
 */

DSTATUS ff_ef_initialize(uint8_t pdrv)
{
    char token;

    ardu_exflash_t *ef = e_flashs[pdrv];

    // 防止重复初始化
    if (!(ef->status & STA_NOINIT))
    {
        return ef->status;
    }

    efUnselect(pdrv);
    efWakeup(pdrv);

    if (efReadDeviceID(pdrv) == ef->deviceID || efReadJedecID(pdrv) == ef->jedecID)
    {
        // efCommand(pdrv, W25QX_WriteStatusReg, true);
        // ef->spi->transfer(0);
        // efUnselect;

        log_i("w25qx init succeed");

        ef->status &= ~STA_NOINIT;
    }
    else
    {
        log_e("w25qx init failed");
    }

    return ef->status;
}

DSTATUS ff_ef_status(uint8_t pdrv)
{
    log_i("ff ef status()");

    if (efReadDeviceID(pdrv) != e_flashs[pdrv]->deviceID)
    {
        log_e("Check status failed");
        return STA_NOINIT;
    }

    return (DSTATUS)e_flashs[pdrv]->status;
}

DRESULT ff_ef_read(uint8_t pdrv, uint8_t *buffer, DWORD sector, UINT count)
{
    log_i("ff ef read()");

    ardu_exflash_t *ef = e_flashs[pdrv];
    if (ef->status & STA_NOINIT)
    {
        log_e("exflash status = sta noinit");
        return RES_NOTRDY;
    }

    DRESULT res = RES_OK;

    // ef->spi->beginTransaction(ef->_settings);

    log_i("read sector address = %x, read count = %d", sector, count);

    for (; count > 0; count--)
    {
        efReadData(pdrv, sector * ef->sectorSize, (uint8_t *)buffer, ef->sectorSize);
        sector++;
        buffer += ef->sectorSize;
    }

    return res;
}

DRESULT ff_ef_write(uint8_t pdrv, const uint8_t *buffer, DWORD sector, UINT count)
{
    log_d("ff ef write()");

    ardu_exflash_t *ef = e_flashs[pdrv];
    if (ef->status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    if (ef->status & STA_PROTECT)
    {
        return RES_WRPRT;
    }

    DRESULT res = RES_OK;

    // ef->spi->beginTransaction(ef->_settings);

    for (; count > 0; count++)
    {
        efWriteData(pdrv, sector * ef->sectorSize, (uint8_t *)buffer, ef->sectorSize);
        sector++;
        buffer += ef->sectorSize;
    }
    return res;
}

DRESULT ff_ef_ioctl(uint8_t pdrv, uint8_t cmd, void *buff)
{
    log_i("ff ef ioctl()");

    ardu_exflash_t *ef = e_flashs[pdrv];

    switch (cmd)
    {
    case CTRL_SYNC:
    {
        if (!efBusy(pdrv))
        {
            log_d("ff sync succeed");
            return RES_OK;
        }
        else
        {
            log_d("ff sync faile");
        }
    }
    case GET_SECTOR_COUNT:
        *((DWORD *)buff) = ef->sectorCount;
        return RES_OK;
    case GET_SECTOR_SIZE:
        *((WORD *)buff) = ef->sectorSize;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = ef->bolckSize;
        return RES_OK;
    }
    return RES_PARERR;
}

/**
 * Public methods
 */

uint8_t exflash_uninit(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];
    if (pdrv >= FF_VOLUMES || ef == NULL)
    {
        return 1;
    }
    efSleep(pdrv);
    ff_diskio_register(pdrv, NULL);
    e_flashs[pdrv] = NULL;
    esp_err_t err = ESP_OK;
    if (ef->base_path)
    {
        err = esp_vfs_fat_unregister_path(ef->base_path);
        free(ef->base_path);
    }
    free(ef);
    return err;
}

/**
 * @brief fatfs function register
 * @param cs spi ss pin
 * @param spi spi classs pointer.
 * @param hz spi frequency.
 * @retval drive number
 */
uint8_t exflash_init(uint8_t cs, SPIClass *spi, int hz)
{
    uint8_t pdrv = 0xFF;

    if (ff_diskio_get_drive(&pdrv) != ESP_OK || pdrv == 0xFF)
    {
        log_e("no available drive number");
        return pdrv;
    }

    /* appy for drive number */
    ardu_exflash_t *flash = (ardu_exflash_t *)malloc(sizeof(ardu_exflash_t));
    if (!flash)
    {
        log_e("flash ardu_exflash_t malloc failed!");
        return 0xFF;
    }
    /* appy for flash read cache */
    uint8_t *cache = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * 4096));
    if (!cache)
    {
        log_e("flash cache malloc failed!");
        free(flash);
        return 0xFF;
    }
    e_readCache = cache;

    flash->_settings = SPISettings(4000000, MSBFIRST, SPI_MODE0);
    flash->ssPin = cs;
    flash->frequency = hz;
    flash->spi = spi;

    flash->base_path = NULL;
    flash->sectorCount = W25QX_SECTOR_COUNT;
    flash->sectorSize = W25QX_SECTOR_SIZE;
    flash->bolckSize = W25QX_BLOCK_SIZE;
    flash->jedecID = W25QX_JEDECID;
    flash->deviceID = W25QX_ID;
    flash->status = STA_NOINIT;

    e_flashs[pdrv] = flash;

    static const ff_diskio_impl_t ef_impl = {
        .init = &ff_ef_initialize,
        .status = &ff_ef_status,
        .read = &ff_ef_read,
        .write = &ff_ef_write,
        .ioctl = &ff_ef_ioctl};

    ff_diskio_register(pdrv, &ef_impl);

    return pdrv;
}

uint8_t exflash_unmount(uint8_t pdrv)
{
    ardu_exflash_t *ef = e_flashs[pdrv];
    if (pdrv >= FF_VOLUMES || ef == NULL)
    {
        return 1;
    }
    ef->status |= STA_NOINIT;

    char drv[3] = {(char)('0' + pdrv), ':', 0};
    f_mount(NULL, drv, 0);
    return 0;
}

bool exflash_mount(uint8_t pdrv, const char *path, uint8_t max_files, bool format_if_empty)
{
    ardu_exflash_t *flash = e_flashs[pdrv];
    if (pdrv >= FF_VOLUMES || flash == NULL)
    {
        return false;
    }

    if (flash->base_path)
    {
        free(flash->base_path);
    }
    flash->base_path = strdup(path);

    FATFS *fs;
    char drv[3] = {(char)('0' + pdrv), ':', 0}; // 获取偏移卷号
    esp_err_t err = esp_vfs_fat_register(path, drv, max_files, &fs);
    if (err == ESP_ERR_INVALID_STATE)
    {
        log_e("esp_vfs_fat_register failed 0x(%x): FLASH is registered.", err);
        return false;
    }
    else if (err != ESP_OK)
    {
        log_e("esp_vfs_fat_register failed 0x(%x)", err);
        return false;
    }

    /* 立即挂载文件系统 */
    FRESULT res = f_mount(fs, drv, 1);
    if (res != FR_OK)
    {
        log_e("f_mount failed: %d", res);
        if (res == 13 && format_if_empty)
        {
            BYTE *work = (BYTE *)malloc(sizeof(BYTE) * FF_MAX_SS);
            log_d("format ff_max_ss = %d", FF_MAX_SS);
            if (!work)
            {
                log_e("malloc for f_mkfs failed");
                return false;
            }

            res = f_mkfs(drv, FM_ANY, 0, work, sizeof(work));
            // free(work);
            if (res != FR_OK)
            {
                log_e("f_mkfs failed: %d", res);
                esp_vfs_fat_unregister_path(path);
                return false;
            }

            res = f_mount(fs, drv, 1);
            if (res != FR_OK)
            {
                log_e("f_mount failed: %d", res);
                esp_vfs_fat_unregister_path(path);
                return false;
            }
        }
        else
        {
            esp_vfs_fat_unregister_path(path);
            return false;
        }
    }

    return true;
}

void exflash_clear(uint8_t pdrv)
{
    efChipErase(pdrv);
}

// void *ff_memalloc(unsigned msize)
// {
//     log_d("ff memalloc");

//     return nullptr;
// }

// void ff_memfree(void *)
// {
//     log_d("ff memfree");
// }