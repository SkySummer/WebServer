#include "static_file.h"

namespace {
    constexpr auto ERROR_HTML_TEMPLATE = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>{0} {1}</title>
    <style>
        body {{ font-family: sans-serif; text-align: center; margin-top: 100px; color: #444; }}
        h1 {{ font-size: 48px; }}
        p {{ font-size: 20px; }}
        a {{ color: #007acc; text-decoration: none; }}
    </style>
</head>
<body>
    <h1>{0} - {1}</h1>
    <p>{2}</p>
    <p><a href="/">Back to Home</a></p>
</body>
</html>
)";
}

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#include <fstream>
#include <unordered_map>

StaticFile::StaticFile(Logger& logger, const std::string_view relative_path) : logger_(logger) {
#ifdef ROOT_PATH
    std::filesystem::path root_path = STR(ROOT_PATH);
#else
    std::filesystem::path root_path = std::filesystem::current_path();
#endif

    root_ = weakly_canonical(root_path / relative_path);
    logger_.log(LogLevel::INFO, std::format("StaticFile initialized. Root: {}", root_.string()));
}

std::string StaticFile::serve(const std::string& path, const Address& info, std::string& status,
                              std::string& content_type) const {
    std::filesystem::path full_path = getFilePath(path);
    logger_.log(LogLevel::DEBUG, info, std::format("Request for static file: {}", full_path.string()));

    if (!isPathSafe(full_path)) {
        // 路径不安全，返回 403
        logger_.log(LogLevel::DEBUG, info, "Path is not safe, return 403.");
        return respondWithError(403, status, content_type);
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        // 找不到文件，返回 404
        logger_.log(LogLevel::DEBUG, info, "Static file not found, return 404.");
        return respondWithError(404, status, content_type);
    }

    logger_.log(LogLevel::DEBUG, info, "Static file found.");

    std::ostringstream ss;
    ss << file.rdbuf();
    status = "200 OK";
    content_type = getMimeType(full_path);
    return ss.str();
}

std::string StaticFile::respondWithError(const int code, std::string& status, std::string& content_type) {
    const auto& [status_text, message] = HttpError::get(code);

    status = std::format("{} {}", code, status_text);
    content_type = "text/html";

    return std::format(ERROR_HTML_TEMPLATE, code, status_text, message);
}

bool StaticFile::isPathSafe(const std::filesystem::path& path) const {
    return weakly_canonical(path).string().starts_with(root_.string());
}

std::filesystem::path StaticFile::getFilePath(const std::string& path) const {
    const std::string clean_path = path == "/" ? "index.html" : path.substr(1);
    return root_ / clean_path;
}

std::string StaticFile::getMimeType(const std::filesystem::path& path) {
    const std::string ext = path.string();
    if (ext.ends_with(".html")) { return "text/html"; }
    if (ext.ends_with(".css")) { return "text/css"; }
    if (ext.ends_with(".js")) { return "application/javascript"; }
    if (ext.ends_with(".png")) { return "image/png"; }
    if (ext.ends_with(".jpg") || ext.ends_with(".jpeg")) { return "image/jpeg"; }
    if (ext.ends_with(".txt")) { return "text/plain"; }
    return "application/octet-stream";
}

const HttpError& HttpError::get(const int code) {
    static const std::unordered_map<int, HttpError> errors = {
        {400, {"Bad Request", "Your request is invalid or malformed."}},
        {403, {"Forbidden", "You don't have permission to access this page."}},
        {404, {"Not Found", "The page you're looking for doesn't exist."}},
        {405, {"Method Not Allowed", "The method you're trying to use is not allowed for this resource."}},
        {500, {"Internal Server Error", "Something went wrong on the server."}},
        {502, {"Bad Gateway", "The server received an invalid response from an upstream server."}},
    };

    if (!errors.contains(code)) {
        return errors.at(500);
    }
    return errors.at(code);
}
