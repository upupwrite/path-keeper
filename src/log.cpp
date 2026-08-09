#include "log.h"

Log::Log() {}

std::string Log::timestamp() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  // 线程安全的 localtime 版本
  std::tm bt;
#ifdef _WIN32
  localtime_s(&bt, &in_time_t);
#else
  localtime_r(&in_time_t, &bt);
#endif

  std::ostringstream oss;
  oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}
