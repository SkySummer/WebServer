#ifndef UTILS_FORM_PARSER_H
#define UTILS_FORM_PARSER_H

#include <sstream>
#include <string>
#include <unordered_map>

#include "utils/url.h"

class FormPasser {
public:
    [[nodiscard]] static std::unordered_map<std::string, std::string> parse(const std::string& body) {
        std::unordered_map<std::string, std::string> result;
        std::istringstream iss(body);
        std::string pair;

        while (std::getline(iss, pair, '&')) {
            const auto pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string key = pair.substr(0, pos);
                std::string value = pair.substr(pos + 1);
                result[Url::decode(key)] = Url::decode(value);
            }
        }

        return result;
    }
};

#endif  // UTILS_FORM_PARSER_H
