#ifndef CORE_EPOLL_MANAGER_H
#define CORE_EPOLL_MANAGER_H

#include <cstdint>
#include <span>

#include <sys/epoll.h>

class EpollManager {
public:
    explicit EpollManager();
    ~EpollManager();

    EpollManager(const EpollManager&) = delete;
    EpollManager& operator=(const EpollManager&) = delete;
    EpollManager(EpollManager&&) = delete;
    EpollManager& operator=(EpollManager&&) = delete;

    void addFd(int fd, uint32_t events) const;  // NOLINT(readability-identifier-length)
    void modFd(int fd, uint32_t events) const;  // NOLINT(readability-identifier-length)
    void delFd(int fd) const;                   // NOLINT(readability-identifier-length)

    [[nodiscard]] int wait(std::span<epoll_event> events, int timeout = -1) const;

    [[nodiscard]] int getEventFd() const;
    [[nodiscard]] int getEpollFd() const;

    void notify() const;

    void clearNotify() const;

private:
    int epoll_fd_;
    int event_fd_;
};

#endif  // CORE_EPOLL_MANAGER_H
