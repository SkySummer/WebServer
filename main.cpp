#include <cstdint>
#include <format>
#include <iostream>

#include "core/server.h"
#include "utils/config_parser.h"
#include "utils/logger.h"

#define STR_HELPER(x) #x      // NOLINT(cppcoreguidelines-macro-usage)
#define STR(x) STR_HELPER(x)  // NOLINT(cppcoreguidelines-macro-usage)

int main() {
    try {
#ifdef ROOT_PATH
        std::filesystem::path root_path = STR(ROOT_PATH);
#else
        std::filesystem::path root_path = std::filesystem::current_path();
#endif

        const ConfigParser config(root_path / "config.ini");

        Logger logger(config.getLogLevel());
        logger.logDivider("Config init");

        const uint16_t port = config.get("port", 8080);
        logger.log(LogLevel::INFO, std::format("Server port: {}", port));

        const size_t thread_count = config.get("thread_count", 4);
        logger.log(LogLevel::INFO, std::format("Thread count: {}", thread_count));

        const bool linger = config.get("linger", true);
        if (linger) {
            logger.log(LogLevel::INFO, "Linger mode enabled.");
        } else {
            logger.log(LogLevel::INFO, "Linger mode disabled.");
        }

        logger.logDivider("Server init");
        Server server(port, linger, thread_count, &logger);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server crashed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
