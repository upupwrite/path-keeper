// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Path Keeper Contributors
// This file is part of Path Keeper.
// Path Keeper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Path Keeper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with Path Keeper. If not, see <https://www.gnu.org/licenses/>.

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
        std::cerr << "Failed to load translation for" << locale.toStdString()
                  << std::endl;
    }

    Interaction interaction;
    interaction.main(argc, argv);

    return 0;
}
