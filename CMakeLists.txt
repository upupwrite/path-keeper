# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Path Keeper Contributors
# This file is part of Path Keeper.
# Path Keeper is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Path Keeper is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with Path Keeper. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)
project(path-keeper VERSION 1.0.1)

# 默认使用 Release 构建类型（若未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (e.g. Debug, Release)" FORCE)
endif()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动 MOC 处理（如需使用 Q_OBJECT 宏）
set(CMAKE_AUTOMOC ON)

# 包含头文件目录
include_directories(include)

# 自动查找 src 目录下的所有 cpp 文件
file(GLOB SOURCES "src/*.cpp")

# 查找 jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp REQUIRED)

# 查找 readline
pkg_check_modules(READLINE readline REQUIRED)

# 查找 Qt（优先 Qt5，降级 Qt6）
find_package(Qt5 COMPONENTS Core LinguistTools QUIET)
if(Qt5_FOUND)
    set(QT_VERSION_MAJOR 5)
    set(QT_LUPDATE_EXECUTABLE ${Qt5_LUPDATE_EXECUTABLE})
    set(QT_LRELEASE_EXECUTABLE ${Qt5_LRELEASE_EXECUTABLE})
    set(QT_CORE_LIBRARY Qt5::Core)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core LinguistTools)
    set(QT_VERSION_MAJOR 6)
    set(QT_LUPDATE_EXECUTABLE ${Qt6_LUPDATE_EXECUTABLE})
    set(QT_LRELEASE_EXECUTABLE ${Qt6_LRELEASE_EXECUTABLE})
    set(QT_CORE_LIBRARY Qt6::Core)
endif()

# 定义翻译文件（.ts）列表
set(TS_FILES
    translations/path-keeper_en.ts
    translations/path-keeper_zh_CN.ts
)

# 根据 Qt 版本调用对应的翻译宏生成 .qm 文件
if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_translation(QM_FILES ${TS_FILES})
elseif(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_translation(QM_FILES ${TS_FILES})
endif()

# 创建自定义目标：更新 .ts 文件（运行 lupdate）
add_custom_target(update_translations
    COMMAND ${QT_LUPDATE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src -ts ${TS_FILES}
    COMMENT "Updating translation source files (ts)"
)

# 创建一个构建目标，确保 .qm 文件在构建可执行文件之前生成
add_custom_target(translations ALL DEPENDS ${QM_FILES})

# 添加可执行文件
add_executable(pk ${SOURCES})

# 链接必要的库：jsoncpp, readline, Qt Core 和 pthread
target_link_libraries(pk PRIVATE
    ${JSONCPP_LIBRARIES}
    ${READLINE_LIBRARIES}
    ${QT_CORE_LIBRARY}
    pthread
)

# 添加包含目录和编译选项
target_include_directories(pk PRIVATE
    ${JSONCPP_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
)
target_compile_options(pk PRIVATE ${READLINE_CFLAGS_OTHER})

# 添加编译器警告选项
target_compile_options(pk PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
)

# 选项：是否构建测试（默认关闭）
option(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
    enable_testing()

    add_executable(test_find_editor
        ./test/test_findInPath.cpp
        ./src/loadfile.cpp
        ./include/loadfile.h
    )
    target_include_directories(test_find_editor PRIVATE ${JSONCPP_INCLUDE_DIRS})
    target_link_libraries(test_find_editor ${JSONCPP_LIBRARIES} pthread)
    add_test(NAME FindNanoTest COMMAND test_find_editor)
endif()

# 安装可执行文件到系统 bin 目录
install(TARGETS pk DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION share/path-keeper/translations)
install(FILES ${CMAKE_SOURCE_DIR}/shell/pk.sh DESTINATION share/path-keeper)


# 创建卸载目标（仅 Unix 系统）
if(UNIX)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_INSTALL_PREFIX}/share/path-keeper/translations
        COMMENT "Uninstalling path-keeper (binary and translations)"
    )
endif()
