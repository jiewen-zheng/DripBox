#ifndef __BOXLOCK_H
#define __BOXLOCK_H

#include "HAL_Def.h"

namespace HAL
{
    class BoxLock
    {
    private:
        /* data */
    public:
        BoxLock(/* args */);
        ~BoxLock();

        void init();

        bool lock();
        void unlock();
    };

}

#endif // __BOXLOCK_H