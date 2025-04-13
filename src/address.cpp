#include "address.h"

#include <utility>
#include <arpa/inet.h>

Address::Address(std::string ip, const uint16_t port): ip_(std::move(ip)), port_(port) {}

Address::Address(const sockaddr_in& addr) {
    char ip_str[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
    ip_ = ip_str;
    port_ = ntohs(addr.sin_port);
}

std::string Address::ip() const {
    return ip_;
}

uint16_t Address::port() const {
    return port_;
}

std::string Address::toString() const {
    return ip_ + ":" + std::to_string(port_);
}

bool Address::operator==(const Address& other) const {
    return ip_ == other.ip_ && port_ == other.port_;
}

bool Address::operator!=(const Address& other) const {
    return !(*this == other);
}
