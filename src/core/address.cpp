#include "core/address.h"

#include <array>
#include <format>
#include <utility>

#include <arpa/inet.h>

Address::Address(std::string ip_address, const uint16_t port, const int conn_fd)
    : ip_(std::move(ip_address)), port_(port), fd_(conn_fd) {}

Address::Address(const sockaddr_in& addr, const int conn_fd) : port_(ntohs(addr.sin_port)), fd_(conn_fd) {
    std::array<char, INET_ADDRSTRLEN> ip_str{};
    inet_ntop(AF_INET, &addr.sin_addr, ip_str.data(), ip_str.size());
    ip_ = ip_str.data();
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
    if (ip_.empty()) {
        return "Unknown";
    }
    return std::format("{}:{}", ip_, port_);
}

bool Address::operator==(const Address& other) const {
    return ip_ == other.ip_ && port_ == other.port_;
}

bool Address::operator!=(const Address& other) const {
    return !(*this == other);
}
