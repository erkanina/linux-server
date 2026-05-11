#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>

class ClientHandler {
public:
    using MessageCallback    = std::function<void(int fd, const std::string& msg)>;
    using DisconnectCallback = std::function<void(int fd)>;

    ClientHandler(int fd, MessageCallback onMsg, DisconnectCallback onDisc);
    ~ClientHandler();

    void start();
    void stop();          // sadece Server::stop() çağırır
    bool send(const std::string& msg) const;
    int  fd() const { return fd_; }

private:
    void readLoop();

    int               fd_;
    std::atomic<bool> running_{ false };
    std::thread       thread_;
    MessageCallback    onMessage_;
    DisconnectCallback onDisconnect_;

    static constexpr size_t BUF = 4096;
};
