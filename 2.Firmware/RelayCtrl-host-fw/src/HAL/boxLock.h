#ifndef __BOXLOCK_H
#define __BOXLOCK_H

#include "HAL_Def.h"

namespace HAL
{
#define UnlockTime 8000U // unlock time 8000 ms

    class BoxLock
    {
    private:
        /* data */
        bool lockState = false;

    public:
        // BoxLock(/* args */);
        // ~BoxLock();

        void init();

        void lock();
        void unlock(bool trig = false);
        void unlock_delay(uint32_t time = 5000);
        bool getClose();
    };

}

#endif // __BOXLOCK_H