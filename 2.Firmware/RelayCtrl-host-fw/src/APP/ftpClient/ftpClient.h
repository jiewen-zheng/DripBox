#ifndef __FTP_CLIENT_H
#define __FTP_CLIENT_H

#include <Arduino.h>
#include <FS.h>

#define TEST_URL "http://192.168.1.232/update/"

class ftpClient
{
private:
    // create buffer for read
    uint8_t buff[1024] = {0};

    String updateURL = TEST_URL;

public:
    void init();

    bool connectWiFi(String ssid, String pass);

    bool isConnected();

    bool getFileToFlash(String path, String fileName);

    void setUpdateURL(String url);
};

extern ftpClient ftp;

#endif