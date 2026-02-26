cmake_minimum_required(VERSION 3.10)
project(path-keeper VERSION 1.0.1)

# 设置C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

# 包含头文件目录
include_directories(include)

# 自动查找src目录下的所有cpp文件
file(GLOB SOURCES "src/*.cpp")

# 查找 jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp REQUIRED)

# 查找 readline
pkg_check_modules(READLINE readline REQUIRED)

# 添加可执行文件
add_executable(pk ${SOURCES})

# 链接 jsoncpp
target_include_directories(pk PRIVATE ${JSONCPP_INCLUDE_DIRS})

# 链接 readline
target_include_directories(pk PRIVATE ${READLINE_INCLUDE_DIRS})
target_compile_options(pk PRIVATE ${READLINE_CFLAGS_OTHER})# test
# enable_testing()

# add_executable(test_find_editor 
#     ./test/test_findInPath.cpp
#     ./src/loadfile.cpp
#     ./include/loadfile.h
# )
# target_include_directories(test_find_editor PRIVATE ${JSONCPP_INCLUDE_DIRS})
# target_link_libraries(test_find_editor  ${JSONCPP_LIBRARIES})
# target_link_libraries(test_find_editor pthread)
# add_test(NAME FindNanoTest 
#          COMMAND test_find_editor)

target_link_libraries(pk PRIVATE ${JSONCPP_LIBRARIES} ${READLINE_LIBRARIES} pthread)

# 添加编译器选项
target_compile_options(pk PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
)

# 安装配置
# 安装可执行文件到系统目录
install(TARGETS pk DESTINATION bin)


# 创建卸载目标
if(UNIX)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        COMMENT "Uninstalling path-keeper"
    )
endif()
