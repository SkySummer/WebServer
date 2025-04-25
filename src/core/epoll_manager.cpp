#include "core/epoll_manager.h"

#include <cstring>
#include <format>
#include <stdexcept>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

EpollManager::EpollManager() : epoll_fd_(epoll_create1(0)), event_fd_(eventfd(0, EFD_NONBLOCK)) {
    if (epoll_fd_ == -1) {
        throw std::runtime_error("Failed to create epoll instance.");
    }

    if (event_fd_ == -1) {
        throw std::runtime_error("Failed to create eventfd.");
    }

    addFd(event_fd_, EPOLLIN);
}

EpollManager::~EpollManager() {
    close(event_fd_);
    close(epoll_fd_);
}

void EpollManager::addFd(const int fd, const uint32_t events) const {  // NOLINT(readability-identifier-length)
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw std::runtime_error(std::format("epoll_ctl ADD failed: {}", strerror(errno)));
    }
}

void EpollManager::modFd(const int fd, const uint32_t events) const {  // NOLINT(readability-identifier-length)
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::runtime_error(std::format("epoll_ctl MOD failed: {}", strerror(errno)));
    }
}

void EpollManager::delFd(const int fd) const {  // NOLINT(readability-identifier-length)
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::runtime_error(std::format("epoll_ctl DEL failed: {}", strerror(errno)));
    }
}

int EpollManager::wait(std::span<epoll_event> events, const int timeout) const {
    return epoll_wait(epoll_fd_, events.data(), static_cast<int>(events.size()), timeout);
}

int EpollManager::getEventFd() const {
    return event_fd_;
}

int EpollManager::getEpollFd() const {
    return epoll_fd_;
}

void EpollManager::notify() const {
    constexpr uint64_t dummy = 1;
    if (write(event_fd_, &dummy, sizeof(dummy)) != sizeof(dummy)) {
        throw std::runtime_error(std::format("Failed to notify: {}", strerror(errno)));
    }
}

void EpollManager::clearNotify() const {
    uint64_t dummy{};
    if (read(event_fd_, &dummy, sizeof(dummy)) != sizeof(dummy)) {
        throw std::runtime_error(std::format("Failed to clear notify: {}", strerror(errno)));
    }
}
