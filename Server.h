#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <map>
#include <mutex>
#include <vector>

class ClientHandler;

class Server {
public:
    using MessageCallback    = std::function<void(int fd, const std::string& msg)>;
    using ConnectionCallback = std::function<void(int fd)>;

    explicit Server(uint16_t port, int backlog = 10);
    ~Server();

    bool start();
    void stop();
    bool sendTo(int fd, const std::string& msg);

    void setOnMessage(MessageCallback cb)       { onMessage_    = std::move(cb); }
    void setOnConnect(ConnectionCallback cb)    { onConnect_    = std::move(cb); }
    void setOnDisconnect(ConnectionCallback cb) { onDisconnect_ = std::move(cb); }

private:
    void listenerLoop();
    void removeClient(int fd);   // sadece map'ten siler, stop() çağırmaz

    uint16_t          port_;
    int               backlog_;
    int               serverFd_{ -1 };
    std::atomic<bool> running_{ false };
    std::thread       listenerThread_;

    std::mutex                                    mtx_;
    std::map<int, std::unique_ptr<ClientHandler>> clients_;

    MessageCallback    onMessage_;
    ConnectionCallback onConnect_;
    ConnectionCallback onDisconnect_;
};
