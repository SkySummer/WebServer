#include "core/static_file.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/http_response.h"
#include "utils/logger.h"
#include "utils/mime_type.h"
#include "utils/url.h"

#define STR_HELPER(x) #x      // NOLINT(cppcoreguidelines-macro-usage)
#define STR(x) STR_HELPER(x)  // NOLINT(cppcoreguidelines-macro-usage)

namespace {
    std::string formatSize(const std::uintmax_t bytes) {
        constexpr std::array<const char*, 5> units = {"B", "KB", "MB", "GB", "TB"};
        constexpr int base = 1024;
        auto size = static_cast<double>(bytes);
        size_t unit_index = 0;

        while (size >= base && unit_index < units.size() - 1) {
            size /= base;
            ++unit_index;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units.at(unit_index);
        return oss.str();
    }

    std::string formatTime(const std::filesystem::file_time_type file_time) {
        const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        const auto raw_time = std::chrono::system_clock::to_time_t(sctp);
        const std::tm local_time = *std::localtime(&raw_time);

        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M");
        return oss.str();
    }
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

std::string StaticFile::serve(const std::string& path, const Address& info) const {
    const std::string decoded_path = Url::decode(path);
    std::filesystem::path full_path = getFilePath(decoded_path);

    logger_->log(LogLevel::DEBUG, info, std::format("Request for static file: {}", full_path.string()));

    if (!isPathSafe(full_path)) {
        // 路径不安全，返回 403
        logger_->log(LogLevel::DEBUG, info, "Path is not safe, return 403.");
        constexpr int error_code = 403;
        return HttpResponse::buildErrorResponse(error_code);
    }

    if (is_directory(full_path)) {
        if (!path.ends_with('/')) {
            std::string corrected_url = path + '/';
            logger_->log(LogLevel::INFO, info,
                         std::format("Redirecting to directory with trailing slash: {} -> {}", path, corrected_url));

            return HttpResponse{}
                .setStatus("301 Moved Permanently")
                .addHeader("Location", corrected_url)
                .setContentType("text/plain")
                .setBody("Redirecting to " + corrected_url)
                .build();
        }

        // 生成目录列表
        logger_->log(LogLevel::DEBUG, info, std::format("Serving directory listing for: {}", full_path.string()));
        return HttpResponse{}
            .setStatus("200 OK")
            .setContentType("text/html; charset=UTF-8")
            .setBody(generateDirectoryListing(full_path, path))
            .build();
    }

    if (auto cached = readFromCache(full_path, info)) {
        // 从缓存中取文件
        logger_->log(LogLevel::DEBUG, info, "Static file served from cache.");
        return cached->build();
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        // 找不到文件，返回 404
        logger_->log(LogLevel::DEBUG, info, "Static file not found, return 404.");
        constexpr int error_code = 404;
        return HttpResponse::buildErrorResponse(error_code);
    }

    std::ostringstream oss;
    oss << file.rdbuf();

    HttpResponse builder;
    builder.setStatus("200 OK").setContentType(MimeType::get(full_path)).setBody(oss.str());

    // 存入缓存
    updateCache(full_path, builder);
    logger_->log(LogLevel::DEBUG, info, "Static file loaded and cached.");

    return builder.build();
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
    std::ranges::sort(directories, filename_less);
    std::ranges::sort(files, filename_less);

    std::ostringstream html;

    html << R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Index of )"
         << Url::decode(request_path) << R"(</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; background-color: #f8f9fa; color: #343a40; padding: 2rem 3rem; }
        h1 { color: #007bff; font-size: 2.5rem; line-height: 1.2; margin-bottom: 2rem; }
        table { width: 100%; border-collapse: collapse; font-size: 1rem; }
        th, td { text-align: left; padding: 0.75rem 1rem; }
        th { background-color: #e9f5ff; border-bottom: 2px solid #007bff; }
        tr:nth-child(even) { background-color: #f1f3f5; }
        a { text-decoration: none; color: #007bff; }
        a:hover { text-decoration: underline; }
        .icon { margin-right: 0.5rem; }
    </style>
</head>
<body>
    <h1>📁 Index of )"
         << Url::decode(request_path) << R"(</h1>
    <table>
        <tr>
            <th>Name</th>
            <th>Size</th>
            <th>Last Modified</th>
        </tr>
)";

    // 返回上级
    if (request_path != "/") {
        html << R"(
        <tr>
            <td><a href="../">⬅️ ../</a></td>
            <td>-</td>
            <td>-</td>
        </tr>
    )";
    }

    std::string base_path = request_path;
    if (!base_path.empty() && base_path.back() != '/') {
        base_path += '/';
    }

    // 目录
    for (const auto& dir : directories) {
        const std::string name = dir.path().filename().string();
        const std::string href = (std::filesystem::path(base_path) / Url::encode(name)).string() + '/';
        const std::string time = formatTime(last_write_time(dir));

        html << std::format(R"(
        <tr>
            <td><a href="{}">📁 {}/</a></td>
            <td>-</td>
            <td>{}</td>
        </tr>
    )",
                            href, name, time);
    }

    // 文件
    for (const auto& file : files) {
        const std::string name = file.path().filename().string();
        const std::string href = (std::filesystem::path(base_path) / Url::encode(name)).string();
        const std::string size = formatSize(file_size(file));
        const std::string time = formatTime(last_write_time(file));

        html << std::format(R"(
        <tr>
            <td><a href="{}">📄 {}</a></td>
            <td>{}</td>
            <td>{}</td>
        </tr>
    )",
                            href, name, size, time);
    }

    html << R"(
    </table>
</body>
</html>
)";

    return html.str();
}

bool StaticFile::isPathSafe(const std::filesystem::path& path) const {
    return weakly_canonical(path).string().starts_with(root_.string());
}

std::filesystem::path StaticFile::getFilePath(const std::string& path) const {
    const std::string clean_path = path == "/" ? "index.html" : path.substr(1);
    return root_ / clean_path;
}

std::optional<HttpResponse> StaticFile::readFromCache(const std::filesystem::path& path, const Address& info) const {
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
    return cache_iter->second.builder;
}

void StaticFile::updateCache(const std::filesystem::path& path, const HttpResponse& builder) const {
    try {
        CacheEntry entry = {.builder = builder, .last_modified = last_write_time(path)};

        std::lock_guard lock(cache_mutex_);
        cache_[path] = std::move(entry);
    } catch (const std::filesystem::filesystem_error& e) {
        // 极端文件丢失情况
        logger_->log(LogLevel::ERROR, std::format("updateCache failed: {} ({})", e.what(), path.string()));
    }
}
