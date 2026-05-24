#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <iostream>

#include "interaction.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTranslator translator;
    QString locale = QLocale::system().name();
    QString appDir = QCoreApplication::applicationDirPath();
    QDir prefixDir(appDir);
    prefixDir.cdUp();
    QString translationPath = prefixDir.absoluteFilePath(
        QString("share/path-keeper/translations/path-keeper_%1.qm")
            .arg(locale));
    if (translator.load(translationPath))
    {
        app.installTranslator(&translator);
    }
    else
    {
        std::cerr << "Failed to load translation for " << locale.toStdString()
                  << std::endl;
    }

    Interaction interaction;
    interaction.main(argc, argv);

    return 0;
}
