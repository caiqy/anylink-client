#include "autostartmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

#if defined(Q_OS_WIN)
#include <QSettings>
#endif

QString AutoStartManager::getAppName()
{
    return QStringLiteral("AnyLink");
}

QString AutoStartManager::getAppPath()
{
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

#if defined(Q_OS_WIN)

bool AutoStartManager::isAutoStartEnabled()
{
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    return settings.contains(getAppName());
}

bool AutoStartManager::setAutoStart(bool enable)
{
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    if (enable) {
        settings.setValue(getAppName(), QStringLiteral("\"%1\"").arg(getAppPath()));
    } else {
        settings.remove(getAppName());
    }
    settings.sync();
    return true;
}

#elif defined(Q_OS_LINUX)

static QString getDesktopFilePath()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return configPath + QStringLiteral("/autostart/anylink.desktop");
}

bool AutoStartManager::isAutoStartEnabled()
{
    return QFile::exists(getDesktopFilePath());
}

bool AutoStartManager::setAutoStart(bool enable)
{
    QString filePath = getDesktopFilePath();

    if (enable) {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QStringLiteral("/autostart"));
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=" << getAppName() << "\n";
        out << "Exec=" << getAppPath() << "\n";
        out << "Icon=anylink\n";
        out << "Comment=AnyLink Secure Client\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        out << "StartupNotify=false\n";
        out << "Terminal=false\n";
        file.close();
        return true;
    } else {
        if (QFile::exists(filePath)) {
            return QFile::remove(filePath);
        }
        return true;
    }
}

#elif defined(Q_OS_MACOS)
// macOS implementation is in autostartmanager_mac.mm
#else

bool AutoStartManager::isAutoStartEnabled()
{
    return false;
}

bool AutoStartManager::setAutoStart(bool enable)
{
    Q_UNUSED(enable);
    return false;
}

#endif
