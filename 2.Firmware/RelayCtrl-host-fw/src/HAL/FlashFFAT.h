#ifndef __HAL_SPIFFS_H
#define __HAL_SPIFFS_H

#include "HAL_Def.h"
#include <FS.h>
#include <vector>

typedef struct
{
    String path;
    String name;
    uint32_t size;
} FileInfo_t;

typedef struct
{
    String path;
    String name;
} DirInfo_t;

namespace HAL
{

    class FlashFFAT
    {
    public:
        fs::FS *fs;

    public:
        // FlashFFAT();
        void init();

        void listDir(const char *dirname, uint8_t levels);
        void createDir(const char *path); // spiffs 不支持目录
        void removeDir(const char *path); // spiffs 不支持目录

        size_t getFileCount(const char *path);
        FileInfo_t getFileInfo(size_t index);

        bool
        findDir(const char *dirPath);
        bool findFile(const char *fileName);

        void readFile(const char *path);
        size_t readFile(const char *path, uint8_t *buf, size_t len);

        void writeFile(const char *path, const char *buf);
        size_t writeFile(const char *path, void *buf, size_t len);

        void appendFile(const char *path, const char *buf);
        void appendFile(const char *path, const uint8_t *buf, size_t size);
        void renameFile(const char *path1, const char *path2);
        void deleteFile(const char *path);
        void deleteAllFile(const char *path);

        void testFileIO(const char *path);

    private:
        std::vector<FileInfo_t> fileList;
        std::vector<DirInfo_t> dirList;
    };

}

#endif
