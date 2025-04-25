#ifndef CORE_STATIC_FILE_H
#define CORE_STATIC_FILE_H

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "core/http_response.h"

struct CacheEntry {
    HttpResponse builder{};                         // 文件内容
    std::filesystem::file_time_type last_modified;  // 最后修改时间
};

// 前向声明
class Address;
class Logger;

class StaticFile {
public:
    explicit StaticFile(Logger* logger, std::string_view relative_path = "./static");

    [[nodiscard]] std::string serve(const std::string& path, const Address& info) const;

private:
    std::filesystem::path root_;  // 静态文件根目录
    Logger* logger_;              // 日志

    mutable std::unordered_map<std::filesystem::path, CacheEntry> cache_;
    mutable std::mutex cache_mutex_;

    [[nodiscard]] bool isPathSafe(const std::filesystem::path& path) const;

    [[nodiscard]] std::filesystem::path getFilePath(const std::string& path) const;

    [[nodiscard]] std::optional<HttpResponse> readFromCache(const std::filesystem::path& path,
                                                            const Address& info) const;

    [[nodiscard]] static std::string generateDirectoryListing(const std::filesystem::path& dir_path,
                                                              const std::string& request_path);

    void updateCache(const std::filesystem::path& path, const HttpResponse& builder) const;
};

#endif  // CORE_STATIC_FILE_H
