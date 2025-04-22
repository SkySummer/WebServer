#include <cstdint>
#include <iostream>

#include "logger.h"
#include "server.h"

int main() {
    try {
        constexpr uint16_t port = 8080;
        constexpr size_t thread_count = 4;

        Logger logger(LogLevel::DEBUG);
        logger.logDivider("Server init");
        Server server(port, thread_count, &logger);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server crashed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
