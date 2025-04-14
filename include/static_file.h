#ifndef STATIC_FILE_H
#define STATIC_FILE_H

#include <string>

#include "logger.h"

struct HttpError {
    std::string status_text;
    std::string message;

    [[nodiscard]] static const HttpError& get(int code);
};

class StaticFile {
public:
    explicit StaticFile(Logger& logger);

    [[nodiscard]] std::string serve(const std::string& path, std::string& status, std::string& content_type) const;

    [[nodiscard]] static std::string respondWithError(int code, std::string& status, std::string& content_type);

private:
    std::string root_; // 静态文件根目录

    Logger& logger_; // 日志

    [[nodiscard]] bool isPathSafe(const std::string& path) const;

    [[nodiscard]] std::string getFilePath(const std::string& path) const;

    [[nodiscard]] static std::string getMimeType(const std::string& path);
};

#endif //STATIC_FILE_H
