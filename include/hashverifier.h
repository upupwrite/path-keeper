#pragma once
#include <string>
#include <json/value.h>

class HashVerifier {
public:
    static std::string sha256(const std::string& data);
    static bool verifyConfig(const Json::Value& config, bool fix = false);
};
