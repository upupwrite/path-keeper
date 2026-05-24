#pragma once
#include <string>
#include <json/value.h>

class HashVerifier {
public:
    static std::string sha256(const std::string& data);
    // 修复：将 config 改为非 const 引用，因为 fix 模式会修改 hashes
    static bool verifyConfig(Json::Value& config, bool fix = false);
};