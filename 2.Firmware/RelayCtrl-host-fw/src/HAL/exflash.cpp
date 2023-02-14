#include "exflash.h"
#include "EF.h"
#include <FS.h>

using namespace HAL;

bool ExFlash::init()
{
    // SPIClass *ef_spi = new SPIClass(FSPI);
    
    if (!EF.begin(EXFLASH_CS))
    {
        Serial.println("exflash mount failed");
        return false;
    }

    return true;
}

void ExFlash::listDir(const char *dirname, uint8_t levels)
{

    File root = EF.open(dirname);
}