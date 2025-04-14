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

#include <filesystem>
#include <fstream>
#include <unordered_map>

StaticFile::StaticFile(Logger& logger) : logger_(logger) {
#ifdef STATIC_ROOT_PATH
    root_ = STR(STATIC_ROOT_PATH);
#else
    root_ = "./static";
#endif

    root_ = std::filesystem::weakly_canonical(root_).string();
    logger_.log(LogLevel::INFO, std::format("StaticFile initialized. Root: {}", root_));
}

std::string StaticFile::serve(const std::string& path, std::string& status, std::string& content_type) const {
    std::string full_path = getFilePath(path);
    logger_.log(LogLevel::DEBUG, std::format("Request for static file: {}", full_path));

    if (!isPathSafe(full_path)) {
        return respondWithError(403, status, content_type);
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        return respondWithError(404, status, content_type);
    }

    logger_.log(LogLevel::DEBUG, std::format("Static file found: {}", full_path));

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

bool StaticFile::isPathSafe(const std::string& path) const {
    return std::filesystem::weakly_canonical(path).string().starts_with(root_);
}

std::string StaticFile::getFilePath(const std::string& path) const {
    const std::string clean_path = path == "/" ? "index.html" : path.substr(1);
    return std::filesystem::path(root_) / clean_path;
}

std::string StaticFile::getMimeType(const std::string& path) {
    if (path.ends_with(".html")) { return "text/html"; }
    if (path.ends_with(".css")) { return "text/css"; }
    if (path.ends_with(".js")) { return "application/javascript"; }
    if (path.ends_with(".png")) { return "image/png"; }
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) { return "image/jpeg"; }
    if (path.ends_with(".txt")) { return "text/plain"; }
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
