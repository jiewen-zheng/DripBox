#include "HAL.h"

HAL::FlashFFAT ffat;
HAL::MainBoard board;
HAL::UartScreen screen(&board);

void HAL::Init()
{
    Serial.begin(115200);
    Serial.println("system init ...");

    ffat.init();
    screen.init();
    board.init();

    ffat.listDir("/", 0);
    if (!ffat.findDir("/config"))
    {
        Serial.println("creat config dir");
        ffat.createDir("/config");
    }

    if (!ffat.findDir("/firmware"))
    {
        Serial.println("creat firmware dir");
        ffat.createDir("/firmware");
    }
}