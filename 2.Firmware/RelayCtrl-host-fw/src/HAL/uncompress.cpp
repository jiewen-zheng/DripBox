#include "uncompress.h"
#include "HAL.h"
#include <FS.h>
#include <FFat.h>

static File file;

static void *Open(const char *filename, int32_t *size)
{
    file = FFat.open(filename);
    *size = file.size();
    return (void *)&file;
}
static void Close(void *p)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    if (f)
        f->close();
}

static int32_t Read(void *p, uint8_t *buffer, int32_t length)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    return f->read(buffer, length);
}

static int32_t Seek(void *p, int32_t position, int iType)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    if (iType == SEEK_SET)
        return f->seek(position);
    else if (iType == SEEK_END)
    {
        return f->seek(position + pzf->iSize);
    }
    else
    { // SEEK_CUR
        long l = f->position();
        return f->seek(l + position);
    }
}

void Uncompress::init()
{
}

bool Uncompress::unzipFile(const char *filePath, const char *unzipPath)
{
    bool succeed = false;
    int code;

    code = zip.openZIP(filePath, Open, Close, Read, Seek);
    if (code != UNZ_OK)
    {
        Serial.printf("[error] zipfile \"%s\" open failed.", filePath);
        return false;
    }

    Serial.printf("Start unzip file :%s\r\n", filePath);

    unz_file_info fi;

    char Comment[512], Name[128];
    zip.getGlobalComment(Comment, sizeof(Comment));
    Serial.print("Global comment: ");
    Serial.println(Comment);
    Serial.println("Files in this archive:");

    uint8_t *buff = (uint8_t *)malloc(sizeof(uint8_t) * 1024);
    if (buff == NULL)
    {
        Serial.println("[error] unzip buff malloc failed");
        zip.closeZIP();
        return false;
    }

    zip.gotoFirstFile();
    do
    { // Display and unzip all files contained in the archive
        code = zip.getFileInfo(&fi, Name, sizeof(Name), NULL, 0, Comment, sizeof(Comment));
        if (code != UNZ_OK)
        {
            break;
        }
        /* debug info */
        Serial.print(Name);
        Serial.print(" - ");
        Serial.print(fi.compressed_size, DEC);
        Serial.print("/");
        Serial.println(fi.uncompressed_size, DEC);

        /* get unziped file size */
        uint32_t size = fi.uncompressed_size;

        String storage = String(unzipPath) + String(Name);

        code = zip.openCurrentFile();
        while (code >= 0 && size)
        {
            code = zip.readCurrentFile(buff, size > 1024 ? 1024 : size);
            if (code > 0)
            {
                ffat.appendFile(storage.c_str(), buff, code);
                size -= code;
            }
            else
            {
                break;
            }
        }
        zip.closeCurrentFile();

        if (size > 0)
        {
            Serial.printf("[error] file \"%s\" unzip failed.\r\n", Name);
            succeed = false;
            break;
        }
        else
        {
            Serial.printf("file \"%s\" unzip succeed.\r\n", Name);
            succeed = true;
        }

        code = zip.gotoNextFile();
    } while (code == UNZ_OK);

    free(buff);

    zip.closeZIP();
    return succeed == true;
}
