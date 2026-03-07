#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QTranslator>

#include "interaction.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 加载翻译文件
    QTranslator translator;
    QString locale = QLocale::system().name();

    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();

    // 向上退一级到安装前缀
    QDir prefixDir(appDir);
    prefixDir.cdUp();

    // 构建翻译文件的完整路径
    QString translationPath = prefixDir.absoluteFilePath(
        QString("share/path-keeper/translations/path-keeper_%1.qm")
            .arg(locale));
    if (translator.load(translationPath))
    {
        app.installTranslator(&translator);
    }
    else
    {
        std::cout << "Failed to load translation for" << locale.toStdString()
                  << std::endl;
    }

    Interaction interaction;
    interaction.main(argc, argv);

    return 0;
}
