#ifndef __HAL_H
#define __HAL_H

#include "HAL_Def.h"
#include "uartScreen.h"
#include "mainBoard.h"
#include "FlashFFAT.h"

#include "FreeRTOS.h"

namespace HAL
{
    void Init();
    void Update();

}
extern HAL::FlashFFAT ffat;
extern HAL::UartScreen screen;
extern HAL::MainBoard board;

#endif