#ifndef UTILS_URL_H
#define UTILS_URL_H

#include <sstream>
#include <string>

class Url {
public:
    [[nodiscard]] static std::string decode(const std::string& url) {
        std::ostringstream decoded;
        for (size_t i = 0; i < url.size(); ++i) {
            if (url[i] == '+') {
                decoded << ' ';
            } else if (url[i] == '%' && i + 2 < url.size()) {
                std::string hex = url.substr(i + 1, 2);
                try {
                    const char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                    decoded << decoded_char;
                    i += 2;
                } catch (...) {
                    decoded << '%';
                }
            } else {
                decoded << url[i];
            }
        }
        return decoded.str();
    }
};

#endif  // UTILS_URL_H
