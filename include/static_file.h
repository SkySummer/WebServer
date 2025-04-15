#ifndef STATIC_FILE_H
#define STATIC_FILE_H

#include <filesystem>
#include <string>

#include "logger.h"

struct HttpError {
    std::string status_text;
    std::string message;

    [[nodiscard]] static const HttpError& get(int code);
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

    [[nodiscard]] bool isPathSafe(const std::filesystem::path& path) const;

    [[nodiscard]] std::filesystem::path getFilePath(const std::string& path) const;

    [[nodiscard]] static std::string getMimeType(const std::filesystem::path& path);
};

#endif //STATIC_FILE_H
