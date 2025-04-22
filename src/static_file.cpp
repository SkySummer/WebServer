#include "static_file.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "logger.h"
#include "utils/mime_type.h"
#include "utils/url.h"

#define STR_HELPER(x) #x      // NOLINT(cppcoreguidelines-macro-usage)
#define STR(x) STR_HELPER(x)  // NOLINT(cppcoreguidelines-macro-usage)

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
}  // namespace

StaticFile::StaticFile(Logger* logger, const std::string_view relative_path) : logger_(logger) {
#ifdef ROOT_PATH
    std::filesystem::path root_path = STR(ROOT_PATH);
#else
    std::filesystem::path root_path = std::filesystem::current_path();
#endif

    root_ = weakly_canonical(root_path / relative_path);
    logger_->log(LogLevel::INFO, std::format("StaticFile initialized. Root: {}", root_.string()));
}

std::string StaticFile::serve(const std::string& path, const Address& info, std::string& status,
                              std::string& content_type) const {
    const std::string decoded_path = Url::decode(path);
    std::filesystem::path full_path = getFilePath(decoded_path);

    logger_->log(LogLevel::DEBUG, info, std::format("Request for static file: {}", full_path.string()));

    if (!isPathSafe(full_path)) {
        // 路径不安全，返回 403
        logger_->log(LogLevel::DEBUG, info, "Path is not safe, return 403.");
        constexpr int error_code = 403;
        return respondWithError(error_code, status, content_type);
    }

    if (std::filesystem::is_directory(full_path)) {
        // 生成目录列表
        logger_->log(LogLevel::DEBUG, info, std::format("Serving directory listing for: {}", full_path.string()));
        content_type = "text/html; charset=UTF-8";
        status = "200 OK";
        return generateDirectoryListing(full_path, path);
    }

    if (auto cached = readFromCache(full_path, status, content_type, info)) {
        // 从缓存中取文件
        logger_->log(LogLevel::DEBUG, info, "Static file served from cache.");
        return *cached;
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        // 找不到文件，返回 404
        logger_->log(LogLevel::DEBUG, info, "Static file not found, return 404.");
        constexpr int error_code = 404;
        return respondWithError(error_code, status, content_type);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    status = "200 OK";
    const std::string content = oss.str();
    content_type = MimeType::get(full_path);

    // 存入缓存
    updateCache(full_path, content, content_type);
    logger_->log(LogLevel::DEBUG, info, "Static file loaded and cached.");

    return content;
}

std::string StaticFile::generateDirectoryListing(const std::filesystem::path& dir_path,
                                                 const std::string& request_path) {
    std::vector<std::filesystem::directory_entry> directories;
    std::vector<std::filesystem::directory_entry> files;

    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_directory()) {
            directories.emplace_back(entry);
        } else {
            files.emplace_back(entry);
        }
    }

    auto filename_less = [](const auto& lhs_entry, const auto& rhs_entry) {
        return lhs_entry.path().filename().string() < rhs_entry.path().filename().string();
    };
    std::sort(directories.begin(), directories.end(), filename_less);
    std::sort(files.begin(), files.end(), filename_less);

    std::ostringstream html;

    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << std::format("<title>Index of {}</title>\n", Url::decode(request_path)) << "<style>\n"
         << "body { font-family: 'Segoe UI', sans-serif; background: #f8f9fa; color: #343a40; padding: 2rem; }\n"
         << "h1 { font-size: 2.5rem; color: #007bff; text-align: center; }\n"
         << "ul { list-style: none; padding: 0; max-width: 600px; margin: 2rem auto; }\n"
         << "li { background: #fff; margin: 0.5rem 0; padding: 0.75rem 1rem; border-radius: 8px; "
         << "box-shadow: 0 1px 3px rgba(0,0,0,0.1); }\n"
         << "a { text-decoration: none; color: #007bff; font-weight: 500; display: block; }\n"
         << "a:hover { text-decoration: underline; }\n"
         << "</style>\n</head>\n<body>\n"
         << std::format("<h1>📂 Index of {}</h1>\n", Url::decode(request_path)) << "<ul>\n";

    // 返回上级
    if (request_path != "/") {
        html << "<li><a href=\"../\">⬅️ ../</a></li>\n";
    }

    std::string base_path = request_path;
    if (!base_path.empty() && base_path.back() != '/') {
        base_path += '/';
    }

    // 目录
    for (const auto& dir : directories) {
        const std::string name = dir.path().filename().string();
        const std::string href = (std::filesystem::path(base_path) / Url::encode(name)).string() + '/';
        html << std::format("<li><a href=\"{}\">📁 {}/</a></li>\n", href, name);
    }

    // 文件
    for (const auto& file : files) {
        const std::string name = file.path().filename().string();
        const std::string href = (std::filesystem::path(base_path) / Url::encode(name)).string();
        html << std::format("<li><a href=\"{}\">📄 {}</a></li>\n", href, name);
    }

    html << "</ul>\n</body>\n</html>\n";
    return html.str();
}

std::string StaticFile::respondWithError(const int code, std::string& status, std::string& content_type) {
    const auto& [status_text, message] = HttpError::get(code);

    status = std::format("{} {}", code, status_text);
    content_type = "text/html; charset=UTF-8";

    return std::format(ERROR_HTML_TEMPLATE, code, status_text, message);
}

bool StaticFile::isPathSafe(const std::filesystem::path& path) const {
    return weakly_canonical(path).string().starts_with(root_.string());
}

std::filesystem::path StaticFile::getFilePath(const std::string& path) const {
    const std::string clean_path = path == "/" ? "index.html" : path.substr(1);
    return root_ / clean_path;
}

std::optional<std::string> StaticFile::readFromCache(const std::filesystem::path& path, std::string& status,
                                                     std::string& content_type, const Address& info) const {
    std::lock_guard lock(cache_mutex_);
    const auto cache_iter = cache_.find(path);

    if (cache_iter == cache_.end()) {
        logger_->log(LogLevel::DEBUG, info, std::format("Cache miss: {}", path.string()));
        return std::nullopt;
    }

    if (!exists(path)) {
        logger_->log(LogLevel::DEBUG, info, std::format("Cache erase (file missing): {}", path.string()));
        cache_.erase(cache_iter);
        return std::nullopt;
    }

    if (cache_iter->second.last_modified != last_write_time(path)) {
        logger_->log(LogLevel::DEBUG, info, std::format("Cache stale: {}", path.string()));
        return std::nullopt;
    }

    logger_->log(LogLevel::DEBUG, info, std::format("Cache hit: {}", path.string()));
    status = "200 OK";
    content_type = cache_iter->second.content_type;
    return cache_iter->second.content;
}

void StaticFile::updateCache(const std::filesystem::path& path, const std::string& content,
                             const std::string& content_type) const {
    try {
        CacheEntry entry = {content, last_write_time(path), content_type};

        std::lock_guard lock(cache_mutex_);
        cache_[path] = std::move(entry);
    } catch (const std::filesystem::filesystem_error& e) {
        // 极端文件丢失情况
        logger_->log(LogLevel::ERROR, std::format("updateCache failed: {} ({})", e.what(), path.string()));
    }
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
        constexpr int default_code = 500;
        return errors.at(default_code);
    }
    return errors.at(code);
}
