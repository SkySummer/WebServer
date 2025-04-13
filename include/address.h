#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>
#include <string>
#include <netinet/in.h>

class Address {
public:
    Address() = default;
    Address(std::string  ip, uint16_t port);
    explicit Address(const sockaddr_in& addr);

    [[nodiscard]] std::string ip() const;
    [[nodiscard]] uint16_t port() const;
    [[nodiscard]] std::string toString() const;

    bool operator==(const Address& other) const;
    bool operator!=(const Address& other) const;

private:
    std::string ip_;
    uint16_t port_{};
};

#endif //ADDRESS_H
