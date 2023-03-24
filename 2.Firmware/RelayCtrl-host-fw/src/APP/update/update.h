#ifndef __UPDATE_H
#define __UPDATE_H

#include "APP/ftpClient/ftpClient.h"

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

class Update : public FtpClient
{
private:
    PackageInfo_t firmware_info;
    PackageInfo_t software_info;

public:
    Update();
    ~Update();

    bool getFirmware();
    void getSoftware();

    void updateScreen();

    void updateMainBoard();

    void updateSelf();
};
#endif
