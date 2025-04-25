#ifndef UTILS_CONFIG_PARSER_H
#define UTILS_CONFIG_PARSER_H

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "utils/logger.h"

class ConfigParser {
public:
    explicit ConfigParser(std::filesystem::path config_file) : config_file_(std::move(config_file)) {
        std::ifstream file(config_file_);
        if (!file.is_open()) {
            std::cerr << "[ConfigParser] Warning: Failed to open config file: " << config_file_
                      << ", using default configuration.\n";
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            trim(line);
            if (line.empty() || line[0] == '#') {
                continue;  // 跳过空行和注释
            }

            const auto pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                trim(key);
                trim(value);
                config_map_[key] = value;
            }
        }
    }

    template <typename T>
    T get(const std::string& key, const T& default_value) const;

    LogLevel getLogLevel() const {
        std::string value = get("log_level", std::string("INFO"));
        if (value == "DEBUG") {
            return LogLevel::DEBUG;
        }
        if (value == "INFO") {
            return LogLevel::INFO;
        }
        if (value == "WARNING") {
            return LogLevel::WARNING;
        }
        if (value == "ERROR") {
            return LogLevel::ERROR;
        }

        return LogLevel::INFO;  // 默认值
    }

private:
    std::unordered_map<std::string, std::string> config_map_;
    std::filesystem::path config_file_;

    static void trim(std::string& str) {
        str.erase(0, str.find_first_not_of(" \t"));
        str.erase(str.find_last_not_of(" \t") + 1);
    }
};

template <typename T>
T ConfigParser::get(const std::string& key, const T& default_value) const {
    T value = default_value;
    if (config_map_.contains(key)) {
        std::istringstream(config_map_.at(key)) >> value;
    }
    return value;
}

template <>
inline bool ConfigParser::get<bool>(const std::string& key, const bool& default_value) const {
    if (config_map_.contains(key)) {
        const std::string value = config_map_.at(key);
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return default_value;
}

#endif  // UTILS_CONFIG_PARSER_H
