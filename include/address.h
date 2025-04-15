#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>
#include <string>
#include <netinet/in.h>

class Address {
public:
    Address() = default;
    Address(std::string ip, uint16_t port, int fd = -1);
    explicit Address(const sockaddr_in& addr, int fd = -1);

    [[nodiscard]] std::string ip() const;
    [[nodiscard]] uint16_t port() const;
    [[nodiscard]] int fd() const;
    [[nodiscard]] std::string toString() const;

    bool operator==(const Address& other) const;
    bool operator!=(const Address& other) const;

private:
    std::string ip_;
    uint16_t port_{};
    int fd_{-1};
};

#endif //ADDRESS_H
