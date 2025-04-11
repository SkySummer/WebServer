#include "server.h"

int main() {
    const Server server(8080);
    server.run();
    return 0;
}
