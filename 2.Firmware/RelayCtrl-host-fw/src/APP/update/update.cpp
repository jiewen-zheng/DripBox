#include "update.h"
#include "HAL/HAL.h"
#include "APP/ftpClient/ftpClient.h"

void update_checkFile()
{
    ffat.listDir("/", 0);

    if (!ffat.isFile("13TouchFile.bin"))
    {
        ftp.getFileToFlash("/", "13TouchFile.bin");
    }
    if (!ffat.isFile("14ShowFile.bin"))
    {
        ftp.getFileToFlash("/", "14ShowFile.bin");
    }
    if (!ffat.isFile("22_Config.bin"))
    {
        ftp.getFileToFlash("/", "22_Config.bin");
    }
    if (!ffat.isFile("32_Image.icl"))
    {
        ftp.getFileToFlash("/", "32_Image.icl");
    }

    if (!ffat.isFile("mainboard.bin"))
    {
        ftp.getFileToFlash("/", "mainboard.bin");
    }
}
