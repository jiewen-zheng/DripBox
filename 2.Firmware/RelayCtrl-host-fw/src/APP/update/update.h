#ifndef __UPDATE_H
#define __UPDATE_H

void update_checkFile();

class Update
{
private:
public:
    Update();
    ~Update();

    bool checkConnect();

    bool reqServer(String url, String msg);
};
#endif
