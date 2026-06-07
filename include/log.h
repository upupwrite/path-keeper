#ifndef LOG_H
#define LOG_H
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

class Log
{
public:
    Log();
    std::string timestamp();
};

#endif  // LOG_H
