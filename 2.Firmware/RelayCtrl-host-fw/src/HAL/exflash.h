#ifndef __EXFLASH_H
#define __EXFLASH_H

#include <Arduino.h>

namespace HAL
{
#define EXFLASH_CS 10

    class ExFlash
    {
    private:
        /* data */
    public:
        // ExFlash(/* args */);
        // ~ExFlash();

        bool init();

        void listDir(const char *dirname, uint8_t levels);
    };
}
#endif
