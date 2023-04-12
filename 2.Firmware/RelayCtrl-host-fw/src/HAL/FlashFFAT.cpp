#include "FlashFFAT.h"
#include <FFat.h>

using namespace HAL;

void FlashFFAT::init()
{
    if (!FFat.begin(true))
    {
        Serial.println("FFat Mount Failed");
        return;
    }

    fs = &FFat;
}

void FlashFFAT::listDir(const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = this->fs->open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    fileList.clear();
    dirList.clear();

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR: ");
            Serial.println(file.name());
            DirInfo_t dirInfo = {
                .path = file.path(),
                .name = file.name(),
            };
            dirList.push_back(dirInfo);

            if (levels)
            {
                listDir(file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.print(file.size());
            Serial.print("\tPATH: ");
            Serial.println(file.path());

            FileInfo_t fileInfo = {
                .path = file.path(),
                .name = file.name(),
                .size = file.size(),
            };
            fileList.push_back(fileInfo);
        }

        file = root.openNextFile();
    }
}

void FlashFFAT::createDir(const char *path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if (fs->mkdir(path))
    {
        Serial.println("- dir created");
    }
    else
    {
        Serial.println("- mkdir failed");
    }
}

void FlashFFAT::removeDir(const char *path)
{
    if (fs->rmdir(path))
    {
        Serial.printf("- dir removed: %s\n", path);
    }
    else
    {
        Serial.printf("- rmdir failed: %s\n", path);
    }
}

size_t HAL::FlashFFAT::getFileCount(const char *path)
{
    listDir(path, 0);

    return fileList.size();
}

FileInfo_t HAL::FlashFFAT::getFileInfo(size_t index)
{
    return fileList.at(index);
}

bool HAL::FlashFFAT::findDir(const char *dirPath)
{
    for (auto iter : dirList)
    {
        if (strcmp(iter.path.c_str(), dirPath) == 0)
            return true;
    }

    return false;
}

bool FlashFFAT::findFile(const char *fileName)
{
    // if (std::find(list.begin(), list.end(), fileName) != list.end())
    // {
    //     return true;
    // }
    // else
    // {
    //     return false;
    // }

    for (auto iter : fileList)
    {
        if (strcmp(iter.name.c_str(), fileName) == 0)
            return true;
    }

    return false;
}

void FlashFFAT::readFile(const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs->open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

size_t FlashFFAT::readFile(const char *path, uint8_t *buf, size_t len)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs->open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return 0;
    }

    size_t read_len = file.read(buf, file.available() > len ? len : file.available());

    file.close();

    return read_len;
}

void FlashFFAT::writeFile(const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs->open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }

    file.close();
}

size_t FlashFFAT::writeFile(const char *path, void *buf, size_t len)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs->open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return 0;
    }

    size_t size = file.write((uint8_t *)buf, len);

    file.close();

    return size;
}

void FlashFFAT::appendFile(const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs->open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
    file.close();
}

void FlashFFAT::appendFile(const char *path, const uint8_t *buf, size_t size)
{
    // Serial.printf("Appending to file: %s\r\n", path);

    File file = fs->open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }

    if (!file.write(buf, size))
    {
        Serial.println("- append failed");
    }

    file.close();
}

void FlashFFAT::renameFile(const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs->rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void FlashFFAT::deleteFile(const char *path)
{
    if (fs->remove(path))
    {
        Serial.printf("- file \"%s\" is deleted successfully.\r\n", path);
    }
    else
    {
        Serial.printf("- file \"%s\" failed to be deleted.\r\n", path);
    }
}

void HAL::FlashFFAT::deleteAllFile(const char *path)
{
    listDir(path, 0);

    for (auto iter : fileList)
    {
        deleteFile(iter.path.c_str());
    }

    for (auto iter : dirList)
    {
        deleteFile(iter.path.c_str());
    }
}

void FlashFFAT::testFileIO(const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs->open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs->open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
    }
}