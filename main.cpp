#include <iostream>

#include "logger.h"
#include "server.h"

int main() {
    try {
        Logger logger(LogLevel::DEBUG);
        Server server(8080, 4, logger);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server crashed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
