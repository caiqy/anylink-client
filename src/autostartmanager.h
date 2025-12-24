#ifndef AUTOSTARTMANAGER_H
#define AUTOSTARTMANAGER_H

#include <QString>

class AutoStartManager
{
public:
    static bool isAutoStartEnabled();
    static bool setAutoStart(bool enable);

private:
    static QString getAppName();
    static QString getAppPath();
};

#endif // AUTOSTARTMANAGER_H
