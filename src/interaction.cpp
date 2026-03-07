#include "interaction.h"

#include <QCoreApplication>
#include <iostream>
#include <string>

#include "colors.h"
#include "info.h"
#include "loadfile.h"

Interaction::Interaction() {}

void Interaction::dir()
{
    std::cout << QCoreApplication::translate("Interaction", "当前所在的目录:")
                     .toStdString()
              << Colors::BLUE << pk.cwd << Colors::RESET << std::endl;
}

void Interaction::main(int argc, char **argv)
{
    try
    {
        if (argc == 1)
        {
            dir();
            pk.runRecent();
            return;
        }

        std::string option = argv[1];

        if (option == "-a" || option == "--add")
        {
            dir();
            pk.addRecord();
        }
        else if (option == "-s" || option == "--show")
        {
            dir();
            pk.showRecord();
        }
        else if (option == "-p" || option == "--point")
        {
            dir();
            if (argc > 2)
            {
                pk.runPoint(argv[2]);
            }
            else
            {
                pk.runPoint();
            }
        }
        else if (option == "-c" || option == "--configure")
        {
            dir();
            pk.setRecent();
        }
        else if (option == "-e" || option == "--execute")
        {
            dir();
            if (argc > 2)
            {
                pk.selectRun(argv[2]);
            }
            else
            {
                pk.selectRun();
            }
        }
        else if (option == "-v" || option == "--version")
        {
            showVersion();
            return;
        }
        else if (option == "--version-verbose" || option == "-V")
        {
            showVersion(true);
            return;
        }
        else if (option == "-h" || option == "--help")
        {
            showHelp();
            return;
        }
        else if (option == "config")
        {
            Editor editor;
            if (argc > 2)
            {
                if (std::strcmp(argv[2], "-editor") == 0)
                {
                    if (argv[3] == nullptr || argv[3][0] == '\0')
                    {
                    SETEDITOR:
                        std::string currentEditor = editor.getEditor();
                        if (!currentEditor.empty())
                        {
                            std::cout
                                << "Current editor setting: " << currentEditor
                                << "\n";
                        }
                        auto editors = editor.getAvailableEditors();

                        if (editors.empty())
                        {
                            std::cout
                                << "No common editors found on your system.\n";
                            std::cout << "Please install one (e.g., vim, nano) "
                                         "and rerun this script.\n";
                            return;
                        }

                        editor.printMenu(editors);
                        std::string selectedEditor =
                            editor.getUserChoice(editors);

                        std::cout << "\nSelected editor: " << selectedEditor
                                  << "\n";
                        editor.setEditor(selectedEditor);
                    }
                    else
                    {
                        std::string selectedEditor = argv[3];
                        std::cout << "Selected editor: " << selectedEditor
                                  << "\n";
                        editor.setEditor(selectedEditor);
                    }
                }
            }
            else
            {
                if (!editor.getEditor().empty())
                {
                    std::string configCommand =
                        editor.getEditor() + " " + Achieve::CONFIG_FILE;
                    pk.shellCommand(configCommand, "~");
                }
                else
                {
                    std::cout << "No current editor setting" << std::endl;
                    goto SETEDITOR;
                }
            }
        }
        else
        {
            std::cout << Colors::RED
                      << QCoreApplication::translate("Interaction",
                                                     "未知选项: ")
                             .toStdString()
                      << option << Colors::RESET << std::endl;
            std::cout << QCoreApplication::translate("Interaction", "用法")
                             .toStdString()
                      << ": pk [-a | -s | -p "
                         "| -c | -e "
                         "[index]]"
                      << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Interaction", "错误: ")
                         .toStdString()
                  << e.what() << Colors::RESET << std::endl;
    }
}
