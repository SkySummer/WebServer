#include "address.h"

#include <format>
#include <utility>
#include <arpa/inet.h>

Address::Address(std::string ip, const uint16_t port, const int fd)
    : ip_(std::move(ip)), port_(port), fd_(fd) {}

Address::Address(const sockaddr_in& addr, const int fd) : fd_(fd) {
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

int Address::fd() const {
    return fd_;
}

std::string Address::toString() const {
    if (ip_.empty()) { return "Unknown"; }
    return std::format("{}:{}", ip_, port_);
}

bool Address::operator==(const Address& other) const {
    return ip_ == other.ip_ && port_ == other.port_;
}

bool Address::operator!=(const Address& other) const {
    return !(*this == other);
}
