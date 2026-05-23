cmake_minimum_required(VERSION 3.10)
project(path-keeper VERSION 1.0.1)

# 默认 Release 构建
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (e.g. Debug, Release)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include_directories(include)

# 自动收集源文件
file(GLOB SOURCES "src/*.cpp")

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp REQUIRED)
pkg_check_modules(READLINE readline REQUIRED)

# Qt 依赖（仅用于翻译）
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

set(TS_FILES
    translations/path-keeper_en.ts
    translations/path-keeper_zh_CN.ts
)

if(QT_VERSION_MAJOR EQUAL 5)
    qt5_add_translation(QM_FILES ${TS_FILES})
elseif(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_translation(QM_FILES ${TS_FILES})
endif()

add_custom_target(update_translations
    COMMAND ${QT_LUPDATE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src -ts ${TS_FILES}
    COMMENT "Updating translation source files (ts)"
)

add_custom_target(translations ALL DEPENDS ${QM_FILES})

add_executable(pk ${SOURCES})

target_link_libraries(pk PRIVATE
    ${JSONCPP_LIBRARIES}
    ${READLINE_LIBRARIES}
    ${QT_CORE_LIBRARY}
)

target_include_directories(pk PRIVATE
    ${JSONCPP_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
)
target_compile_options(pk PRIVATE ${READLINE_CFLAGS_OTHER})

target_compile_options(pk PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
)

# 安装目标
install(TARGETS pk DESTINATION bin)
install(FILES ${QM_FILES} DESTINATION share/path-keeper/translations)
install(FILES ${CMAKE_SOURCE_DIR}/shell/pk.sh DESTINATION share/path-keeper)

# 测试
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
