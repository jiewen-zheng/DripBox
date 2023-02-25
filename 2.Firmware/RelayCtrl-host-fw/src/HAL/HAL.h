#ifndef __HAL_H
#define __HAL_H

#include "HAL_Def.h"
#include "uartSCR.h"
#include "mainBoard.h"
#include "FlashFFAT.h"

namespace HAL
{
    void Init();
    void Update();
}
extern HAL::UART_SCR scr;
extern HAL::MainBoard board;
extern HAL::FlashFFAT ffat;
#endif