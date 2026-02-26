#include <iostream>
#include <ostream>

#include "loadfile.h"

int main()
{
    Editor editor;
    std::cout << "输入查找nano\n";
    auto result = editor.findInPath("nano");

    if (result.empty())
    {
        std::cout << "找到的nano路径" << result << std::endl;
        return 1;
    }
    else
    {
        std::cout << "找到的nano路径" << result << std::endl;
    }

    auto editors = editor.getAvailableEditors();
    std::cout << editors.size() << std::endl;
    if (editors.size() == 0)
    {
        std::cout << "无法正确找到编辑器路径\n";
        return 1;
    }
    for (size_t i = 0; i < editors.size(); ++i)
    {
        std::cout << i + 1 << ". " << editors[i].first
                  << " (路径: " << editors[i].second << ")\n";
    }
}
