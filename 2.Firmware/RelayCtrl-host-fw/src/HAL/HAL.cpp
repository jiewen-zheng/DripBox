#include "HAL.h"

HAL::FlashFFAT ffat;
HAL::MainBoard board;
HAL::UartScreen screen(&board);

void HAL::Init()
{
    Serial.begin(115200);
    Serial.println("system init ...");

    ffat.init();
    board.init();
    screen.init();

    // Update display in parallel thread.

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