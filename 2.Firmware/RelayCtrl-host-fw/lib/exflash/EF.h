#ifndef __EF_H
#define __EF_H

#include "FS.h"
#include <SPI.H>

namespace fs
{
    class EXFLASH : public FS
    {
    protected:
        uint8_t _pdrv;

    public:
        EXFLASH(FSImplPtr impl);
        bool begin(uint8_t ssPin = SS, SPIClass &spi = SPI, uint32_t frequency = 4000000, const char *mountpoint = "/exffs", uint8_t max_files = 5, bool format_if_empty = true);
        void end();

        void clear();
    };
}

extern fs::EXFLASH EF;

#endif