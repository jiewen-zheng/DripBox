#include "vfs_api.h"
#include "ef_diskio.h"
#include "ff.h"
#include "FS.h"
#include "EF.h"

using namespace fs;

EXFLASH::EXFLASH(FSImplPtr impl) : FS(impl), _pdrv(0xff)
{
}

bool EXFLASH::begin(uint8_t ssPin, SPIClass &spi, uint32_t frequency, const char *mountpoint, uint8_t max_files,
                    bool format_if_empty)
{
    if (_pdrv != 0xFF)
    {
        log_i("ex_flash _pdrv = %d", _pdrv);
        return true;
    }

    // #if defined(W25QX_SCLK) && defined(W25QX_MISO) && defined(W25QX_MOSI)
    //     spi.begin(W25QX_SCLK, W25QX_MISO, W25QX_MOSI);
    // #else
    //     spi.begin();
    // #endif
    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, HIGH);

    spi.begin();

    _pdrv = exflash_init(ssPin, &spi, frequency);
    if (_pdrv == 0xFF)
    {
        log_e("ex_flash drive number = 0xff");
        return false;
    }

    if (!exflash_mount(_pdrv, mountpoint, max_files, format_if_empty))
    {
        exflash_unmount(_pdrv);
        exflash_uninit(_pdrv);
        _pdrv = 0xFF;
        return false;
    }

    _impl->mountpoint(mountpoint);
    return true;
}

void EXFLASH::end()
{
    if (_pdrv != 0xFF)
    {
        _impl->mountpoint(NULL);
        exflash_unmount(_pdrv);
        exflash_uninit(_pdrv);

        _pdrv = 0xFF;
    }
}

void EXFLASH::clear()
{
    exflash_clear(_pdrv);
}

EXFLASH EF = EXFLASH(FSImplPtr(new VFSImpl()));