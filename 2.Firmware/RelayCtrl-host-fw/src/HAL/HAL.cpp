#include "HAL.h"

HAL::UART_SCR scr;
HAL::MainBoard board;
HAL::FlashFFAT ffat;

void HAL::Init()
{
    Serial.begin(115200);
    Serial.println("system init ...");

    ffat.init();
    scr.init();
    board.init();

    // ffat.createDir("/dirrr");
    // ffat.listDir("/", 1);
    // ffat.writeFile("/dirrr/name.txt", "haha");
    // ffat.readFile("/dirrr/name.txt");
    // ffat.listDir("/", 1);

    // ffat.deleteFile("/dirrr/name.txt");
    // ffat.listDir("/", 1);
    // ffat.removeDir("/dirrr");
    // ffat.listDir("/", 1);
}