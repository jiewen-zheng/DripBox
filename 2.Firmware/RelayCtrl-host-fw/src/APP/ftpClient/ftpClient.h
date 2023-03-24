#ifndef __FTP_CLIENT_H
#define __FTP_CLIENT_H

#include <Arduino.h>
#include <FS.h>

class FtpClient
{
private:
    // create buffer for read
    uint8_t buff[1024] = {0};

public:
    bool checkConnect();

    bool getFileToFlash(String path, String fileName);

    String reqServer(String url, String msg);

protected:
};

#endif