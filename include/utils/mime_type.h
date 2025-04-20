#ifndef UTILS_MIME_TYPE_H
#define UTILS_MIME_TYPE_H

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace detail {
    inline const std::unordered_map<std::string_view, std::string> MIME_MAP = {
        {".html", "text/html; charset=UTF-8"},
        {".htm", "text/html; charset=UTF-8"},
        {".css", "text/css; charset=UTF-8"},
        {".js", "application/javascript; charset=UTF-8"},
        {".json", "application/json; charset=UTF-8"},
        {".xml", "application/xml; charset=UTF-8"},
        {".txt", "text/plain; charset=UTF-8"},
        {".csv", "text/csv; charset=UTF-8"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"},
        {".webp", "image/webp"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".rar", "application/vnd.rar"},
        {".mp3", "audio/mpeg"},
        {".mp4", "video/mp4"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},
    };

    [[nodiscard]] inline std::string getMime(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::ranges::transform(ext, ext.begin(), tolower);

        if (const auto mime_iter = MIME_MAP.find(ext); mime_iter != MIME_MAP.end()) {
            return mime_iter->second;
        }
        return "application/octet-stream";
    }
}  // namespace detail

class MimeType {
public:
    [[nodiscard]] static std::string get(const std::filesystem::path& path) { return detail::getMime(path); }
};

#endif  // UTILS_MIME_TYPE_H
