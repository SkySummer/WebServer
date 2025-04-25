#include "core/http_response.h"

#include <format>
#include <map>
#include <sstream>
#include <string>

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

HttpResponse& HttpResponse::setStatus(const std::string& status) {
    status_ = status;
    return *this;
}

HttpResponse& HttpResponse::setContentType(const std::string& type) {
    headers_["Content-Type"] = type;
    return *this;
}

HttpResponse& HttpResponse::setBody(const std::string& body) {
    body_ = body;
    return *this;
}

HttpResponse& HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

std::string HttpResponse::build() {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_ << "\r\n";
    headers_["Content-Length"] = std::to_string(body_.size());
    headers_["Connection"] = "close";

    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n" << body_;
    return oss.str();
}

std::string HttpResponse::buildErrorResponse(const int code, const std::string& tips) {
    std::string status;
    std::string message;

    // NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
    switch (code) {
        case 400:
            status = "Bad Request";
            message = "Your request is invalid or malformed.";
            break;
        case 403:
            status = "Forbidden";
            message = "You don't have permission to access this page.";
            break;
        case 404:
            status = "Not Found";
            message = "The page you're looking for doesn't exist.";
            break;
        case 405:
            status = "Method Not Allowed";
            message = "The method you're trying to use is not allowed for this resource.";
            break;
        case 500:
            status = "Internal Server Error";
            message = "Something went wrong on the server.";
            break;
        case 502:
            status = "Bad Gateway";
            message = "The server received an invalid response from an upstream server.";
            break;
        default:
            status = "Unknown Error";
            message = std::to_string(code) + " Unknown Error";
            break;
    }
    // NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

    if (!tips.empty()) {
        message += " " + tips;
    }

    return HttpResponse{}
        .setStatus(std::format("{} {}", code, status))
        .setContentType("text/html; charset=UTF-8")
        .setBody(std::format(ERROR_HTML_TEMPLATE, code, status, message))
        .build();
}
