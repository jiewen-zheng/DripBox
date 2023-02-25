#ifndef __UNCOMPRESS_H
#define __UNCOMPRESS_H

#include <unzipLIB.h>

#include <stdint.h>

class Uncompress
{
private:
    UNZIP zip;

public:
    void init();

    bool unzipFile(const char *filePath, const char *unzipPath);
};
#endif