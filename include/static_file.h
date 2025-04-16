#ifndef STATIC_FILE_H
#define STATIC_FILE_H

#include <filesystem>
#include <string>
#include <unordered_map>

#include "logger.h"

struct HttpError {
    std::string status_text; // 状态码
    std::string message; // 信息

    [[nodiscard]] static const HttpError& get(int code);
};

struct CacheEntry {
    std::string content; // 文件内容
    std::filesystem::file_time_type last_modified; // 文件最后修改时间
    std::string content_type; // MIME 类型
};

class StaticFile {
public:
    explicit StaticFile(Logger& logger, std::string_view relative_path = "./static");

    [[nodiscard]] std::string serve(const std::string& path, const Address& info, std::string& status,
                                    std::string& content_type) const;

    [[nodiscard]] static std::string respondWithError(int code, std::string& status, std::string& content_type);

private:
    std::filesystem::path root_; // 静态文件根目录
    Logger& logger_; // 日志

    mutable std::unordered_map<std::filesystem::path, CacheEntry> cache_;
    mutable std::mutex cache_mutex_;

    [[nodiscard]] bool isPathSafe(const std::filesystem::path& path) const;

    [[nodiscard]] std::filesystem::path getFilePath(const std::string& path) const;

    std::optional<std::string> readFromCache(const std::filesystem::path& path, std::string& status,
                                             std::string& content_type, const Address& info) const;

    void updateCache(const std::filesystem::path& path, const std::string& content,
                     const std::string& content_type) const;
};

#endif //STATIC_FILE_H
