#ifndef __FTP_CLIENT_H
#define __FTP_CLIENT_H

#include <Arduino.h>
#include <FS.h>

#define FIRMWARE_REQ_URL "http://59.110.138.60:8008/fileInfo/selectOne?phoneModel=ROTEX-Z003&fileType=0"
#define SOFTWARE_REQ_URL "http://59.110.138.60:8008/fileInfo/selectOne?phoneModel=ROTEX-Z003&fileType=1"

typedef struct
{
    String fileName;
    int fileType;
    String fileUrl;
    String phoneModel;
    int version;
} PackageInfo_t;

typedef enum
{
    NOT_UPDATE,
    SERVER_ERROR,
    FILE_ERROR,
    REQ_OK,
} FtpState_e;

class FtpClient
{
private:
    // create buffer for read
    uint8_t buff[1024] = {0};

    PackageInfo_t firmware_info;
    PackageInfo_t software_info;

public:
    bool checkConnect();

    bool getFileToFlash(String path, String fileName, String url);

    String reqServer(String url, String msg);

    bool getUpdateFileUrl(PackageInfo_t *info, String req_url);

    FtpState_e getFirmware(int now_ver);
    bool firmwareDownload();
    PackageInfo_t getFirmwareInfo();
    int getFirmwareVersion();

    FtpState_e getSoftware(int now_ver);
    String getSoftwareURL();
    PackageInfo_t getSoftwareInfo();
    int getSoftwareVersion();

protected:
};

#endif