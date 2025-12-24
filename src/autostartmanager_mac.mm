#include "autostartmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

#if defined(Q_OS_MACOS)

static QString getPlistFilePath()
{
    QString homePath = QDir::homePath();
    return homePath + QStringLiteral("/Library/LaunchAgents/pro.anylink.client.plist");
}

bool AutoStartManager::isAutoStartEnabled()
{
    return QFile::exists(getPlistFilePath());
}

bool AutoStartManager::setAutoStart(bool enable)
{
    QString filePath = getPlistFilePath();

    if (enable) {
        QDir dir(QDir::homePath() + QStringLiteral("/Library/LaunchAgents"));
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QString appPath = getAppPath();

        QTextStream out(&file);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
        out << "<plist version=\"1.0\">\n";
        out << "<dict>\n";
        out << "    <key>Label</key>\n";
        out << "    <string>pro.anylink.client</string>\n";
        out << "    <key>ProgramArguments</key>\n";
        out << "    <array>\n";
        out << "        <string>" << appPath << "</string>\n";
        out << "    </array>\n";
        out << "    <key>RunAtLoad</key>\n";
        out << "    <true/>\n";
        out << "</dict>\n";
        out << "</plist>\n";
        file.close();
        return true;
    } else {
        if (QFile::exists(filePath)) {
            return QFile::remove(filePath);
        }
        return true;
    }
}

#endif
