#pragma once
#include <ctime>
#include <iostream>
#include <string>

const std::string BUILD_DATE = __DATE__ " " __TIME__;
const std::string COMPILER =
#ifdef __clang__
    "Clang " __clang_version__;
#elif __GNUC__
    "GCC " __VERSION__;
#elif _MSC_VER
    "MSVC " + std::to_string(_MSC_VER);
#else
    "Unknown Compiler";
#endif

void showVersion(bool verbose = false);
void showHelp();
