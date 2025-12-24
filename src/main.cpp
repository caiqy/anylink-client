#include "anylink.h"
#include "common.h"
#include "configmanager.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QTranslator>
#include <QStyleFactory>
#include <QPalette>
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_WIN)
#include "singleapplication.h"
#endif
#include <QDesktopServices>

void outdateCheck()
{
    if (QDate::currentDate().daysTo(QDate(2024,5,1)) < 0) {
        error(QObject::tr("The current version of the software has expired, please install the latest version!"));
    }
}

void setLightTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(0, 122, 204));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));

    app.setPalette(lightPalette);
}

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{type}:[%{file}:%{line}]  %{message}");
    QApplication::setApplicationName("AnyLink");
    configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
//    qDebug() << configLocation;
    tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(configLocation);

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    SingleApplication app(argc, argv);
#else
    QApplication app(argc, argv);
#endif

    setLightTheme(app);

    configManager = new ConfigManager();
    // Multiple translation files can be installed.
    // Translations are searched for in the reverse order in which they were installed
    QTranslator myTranslator; // must global

    if(configManager->loadConfig(Json)) {
        if(configManager->config["local"].toBool()) {
//            qDebug() << QLocale::system().name();
            // embedded in qrc
            if(myTranslator.load(QLocale(), QLatin1String("anylink"), QLatin1String("_"), QLatin1String(":/i18n"))) {
                app.installTranslator(&myTranslator);
            }
        }
    }

//    outdateCheck();

    AnyLink w;
    w.show();

    QApplication::setQuitOnLastWindowClosed(false);
    QObject::connect(&app, &SingleApplication::instanceStarted, &w, &AnyLink::showNormal);

    return app.exec();
}
