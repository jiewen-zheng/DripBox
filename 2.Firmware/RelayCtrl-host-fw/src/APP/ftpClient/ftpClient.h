#ifndef __FTP_CLIENT_H
#define __FTP_CLIENT_H

#include <Arduino.h>
#include <FS.h>

#define TEST_URL "http://192.168.1.232/update/"
#define SERVER_URL "http://59.110.138.60:8008/fileInfo/selectOne"

class ftpClient
{
private:
    // create buffer for read
    uint8_t buff[1024] = {0};

    String serverURL = TEST_URL;

    typedef struct
    {
        String url_scr;
        String url_self;
        String url_board;
    } FileUrl_t;
    FileUrl_t fileUrl;

public:
    void init();

    bool connectWiFi(String ssid, String pass);

    bool checkConnect();

    bool getFileUrl();

    bool getFileToFlash(String path, String fileName);

    void setServerUrl(String url);

protected:
    bool reqServer(String url, String msg);
};

extern ftpClient ftp;

#endif