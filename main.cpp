#include "server.h"

int main() {
    Server server(8080, 4);
    server.run();
    return 0;
}
