#pragma once
#include <string>
#include <json/value.h>
#include <ctime>

class Logger {
    bool enabled_;
    std::string log_dir_;
    int max_age_days_;
    Json::Value config_;
public:
    Logger() : enabled_(false), max_age_days_(30) {}
    explicit Logger(const Json::Value& config) {
        config_ = config;
        if (config.isMember("log")) {
            enabled_ = config["log"].get("enabled", false).asBool();
            log_dir_ = config["log"].get("dir", "~/.pk_logs").asString();
            max_age_days_ = config["log"].get("max_age_days", 30).asInt();
        }
    }
    bool enabled() const { return enabled_; }
    std::string getLogFilePath() const;
    std::string getLogDir() const;
    void cleanupOldLogs() const;  // 直接删除旧文件
    void listLogs() const;        // 输出到 stderr
};

// 实现见 logger.cpp
