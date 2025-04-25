#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel : std::uint8_t {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

// 前向声明
class Address;

class Logger {
public:
    // 构造函数：打开日志文件并设置最低日志等级
    explicit Logger(LogLevel min_level = LogLevel::INFO);

    // 析构函数：确保日志文件关闭
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // 写入一条日志信息
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const Address& address, const std::string& message);

    // 写入一条分隔符
    void logDivider(const std::string& title, LogLevel level = LogLevel::INFO);

private:
    std::ofstream file_;
    std::string filename_;
    std::mutex mutex_;
    LogLevel min_level_;

    // 基于当前日期生成日志文件名
    [[nodiscard]] static std::string generateLogFilename();

    // 日期轮换
    void rotateIfNeeded();

    // 获取当前时间
    [[nodiscard]] static std::string currentTime();

    // 将日志等级枚举值转换为字符串形式
    [[nodiscard]] static std::string logLevelToString(LogLevel level);
};

#endif  // UTILS_LOGGER_H
