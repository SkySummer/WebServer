#include "utils/logger.h"

#include <chrono>
#include <iomanip>
#include <iostream>

#include "core/address.h"

Logger::Logger(const LogLevel min_level) : min_level_(min_level) {
    filename_ = generateLogFilename();
    file_.open(filename_, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file " + filename_);
    }
}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::log(const LogLevel level, const std::string& message) {
    if (level < min_level_ || !file_.is_open()) {
        return;
    }

    try {
        rotateIfNeeded();
    } catch (const std::exception& e) {
        std::cerr << "Logger::log(): Failed to rotate log file: " << e.what() << '\n';
        return;
    }

    const std::string time = currentTime();

    std::lock_guard lock(mutex_);

    // 写入日志格式：[YYYY-MM-DD HH:MM:SS] [LEVEL] message
    file_ << std::format("[{}] [{}] {}", time, logLevelToString(level), message) << '\n';
    file_.flush();
}

void Logger::log(const LogLevel level, const Address& address, const std::string& message) {
    if (level < min_level_ || !file_.is_open()) {
        return;
    }

    try {
        rotateIfNeeded();
    } catch (const std::exception& e) {
        std::cerr << "Logger::log(): Failed to rotate log file: " << e.what() << '\n';
        return;
    }

    const std::string time = currentTime();
    const std::string client_info = address.toString();
    const int client_fd = address.fd();

    std::lock_guard lock(mutex_);

    if (client_fd != -1) {
        file_ << std::format("[{}] [{}] [Client {}] [fd: {}] {}", time, logLevelToString(level), client_info, client_fd,
                             message)
              << '\n';
    } else {
        file_ << std::format("[{}] [{}] [Client {}] {}", time, logLevelToString(level), client_info, message) << '\n';
    }
    file_.flush();
}

void Logger::logDivider(const std::string& title, const LogLevel level) {
    const std::string line = "========== " + title + " ==========";
    log(level, line);
}

std::string Logger::generateLogFilename() {
    // 获取当前日期
    const auto now = std::chrono::system_clock::now();
    const std::chrono::zoned_time local_time(std::chrono::current_zone(), now);
    const auto today = std::chrono::floor<std::chrono::days>(local_time.get_local_time());
    const std::chrono::year_month_day ymd(today);

    const int year = static_cast<int>(ymd.year());
    const unsigned month = static_cast<unsigned>(ymd.month());
    const unsigned day = static_cast<unsigned>(ymd.day());

    return std::format("log_{:04}-{:02}-{:02}.log", year, month, day);
}

void Logger::rotateIfNeeded() {
    std::lock_guard lock(mutex_);
    if (std::string filename = generateLogFilename(); filename_ != filename) {
        if (file_.is_open()) {
            file_.close();
        }
        filename_ = std::move(filename);
        file_.open(filename_, std::ios::app);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open log file " + filename_);
        }
    }
}

std::string Logger::logLevelToString(const LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::currentTime() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%F %T");
    return oss.str();
}
