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


# ======================================================================
# Rust editor library
# ======================================================================

find_program(CARGO_EXECUTABLE cargo REQUIRED)

set(RUST_EDITOR_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/rust/editor")
set(RUST_EDITOR_TARGET   "${CMAKE_CURRENT_BINARY_DIR}/rust-editor")
set(RUST_EDITOR_LIB      "${RUST_EDITOR_TARGET}/release/libeditor.a")

# Cargo profile: build Debug when CMAKE_BUILD_TYPE is Debug, else Release.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RUST_EDITOR_PROFILE "debug")
    set(RUST_EDITOR_CARGO_ARGS "")
else()
    set(RUST_EDITOR_PROFILE "release")
    set(RUST_EDITOR_CARGO_ARGS "--release")
endif()

# Re-run cargo whenever any source or manifest changes. The glob is expanded
# at configure time; adding new .rs files requires re-running cmake (which is
# normal CMake behaviour).
file(GLOB_RECURSE RUST_EDITOR_SOURCES CONFIGURE_DEPENDS
    "${RUST_EDITOR_DIR}/src/*.rs"
)

add_custom_command(
    OUTPUT  "${RUST_EDITOR_LIB}"
    COMMAND "${CMAKE_COMMAND}" -E env
            "CARGO_TARGET_DIR=${RUST_EDITOR_TARGET}"
            "${CARGO_EXECUTABLE}" build ${RUST_EDITOR_CARGO_ARGS}
            --manifest-path "${RUST_EDITOR_DIR}/Cargo.toml"
    DEPENDS
        "${RUST_EDITOR_DIR}/Cargo.toml"
        "${RUST_EDITOR_DIR}/Cargo.lock"
        ${RUST_EDITOR_SOURCES}
    WORKING_DIRECTORY "${RUST_EDITOR_DIR}"
    COMMENT "Building Rust editor library (${RUST_EDITOR_PROFILE})"
    VERBATIM
)

add_custom_target(editor_rust_build DEPENDS "${RUST_EDITOR_LIB}")

# Wrap the .a in an INTERFACE-style imported target so we can link against it
# with target_link_libraries() just like any other library.
add_library(editor_rust STATIC IMPORTED GLOBAL)
add_dependencies(editor_rust editor_rust_build)
set_target_properties(editor_rust PROPERTIES
    IMPORTED_LOCATION "${RUST_EDITOR_LIB}"
)

# Rust std needs these on Linux/macOS. On Windows MSVC the equivalent set is
# ws2_32, userenv, bcrypt, ntdll — added conditionally below.
if(WIN32)
    set(RUST_EDITOR_SYSTEM_LIBS ws2_32 userenv bcrypt ntdll)
else()
    set(RUST_EDITOR_SYSTEM_LIBS pthread dl m)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    if(APPLE AND CORE_FOUNDATION_LIBRARY)
        list(APPEND RUST_EDITOR_SYSTEM_LIBS ${CORE_FOUNDATION_LIBRARY})
    endif()
endif()

target_link_libraries(editor_rust INTERFACE ${RUST_EDITOR_SYSTEM_LIBS})

# ======================================================================
# end Rust editor library
# ======================================================================

# 默认使用 Release 构建类型（若未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (e.g. Debug, Release)" FORCE)
endif()

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用自动 MOC 处理（如需使用 Q_OBJECT 宏）
set(CMAKE_AUTOMOC ON)

set(SOURCES
    src/info.cpp
    src/interaction.cpp
    src/loadfile.cpp
    src/main.cpp
    src/pk.cpp
    src/terminal.cpp
    src/search.cpp
    src/log.cpp
)

set(HEADERS
    include/colors.h
    include/info.h
    include/interaction.h
    include/loadfile.h
    include/pk.h
    include/readline.h
    include/search.h
    include/terminal.h
    include/log.h
    rust/editor/include/editor.h
)

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
add_executable(pk ${SOURCES} ${HEADERS})

# 链接必要的库：jsoncpp, readline, Qt Core 和 pthread
target_link_libraries(pk PRIVATE
    editor_rust
    ${JSONCPP_LIBRARIES}
    ${READLINE_LIBRARIES}
    ${QT_CORE_LIBRARY}
    pthread
)

# 添加包含目录和编译选项
target_include_directories(pk PRIVATE
    ${JSONCPP_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/rust/editor/include
)
target_compile_options(pk PRIVATE ${READLINE_CFLAGS_OTHER})

# 添加编译器警告选项
target_compile_options(pk PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
)

option(BUILD_TESTS "Build the tests" OFF)
if(BUILD_TESTS)
    message(STATUS "Building tests: ENABLED")
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(
        NAME pk_tests
        COMMAND ${Python3_EXECUTABLE} -m pytest -s -v ${CMAKE_CURRENT_SOURCE_DIR}/test/test.py
    )
    set_tests_properties(pk_tests PROPERTIES
        ENVIRONMENT "PK_BINARY=$<TARGET_FILE:pk>;PYTHONUNBUFFERED=1"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    message(STATUS "Building tests: DISABLED")
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

# Clean up the cargo target directory on uninstall.
if(UNIX)
    add_custom_target(uninstall_rust
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${RUST_EDITOR_TARGET}"
        COMMENT "Removing Rust editor build directory"
    )
    add_dependencies(uninstall uninstall_rust)
endif()
