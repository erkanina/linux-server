#include "Server.h"
#include "ClientHandler.h"
#include "Logger.h"
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server(uint16_t port, int backlog) : port_(port), backlog_(backlog) {}
Server::~Server() { stop(); }

bool Server::start()
{
    serverFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ < 0) { LOG("[Server] socket: ", std::strerror(errno)); return false; }

    int opt = 1;
    ::setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port_);

    if (::bind(serverFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG("[Server] bind: ", std::strerror(errno)); ::close(serverFd_); return false;
    }
    if (::listen(serverFd_, backlog_) < 0) {
        LOG("[Server] listen: ", std::strerror(errno)); ::close(serverFd_); return false;
    }

    running_ = true;
    listenerThread_ = std::thread(&Server::listenerLoop, this);
    LOG("[Server] Port ", port_, " dinleniyor.");
    return true;
}

void Server::stop()
{
    if (!running_.exchange(false)) return;
    LOG("[Server] Durduruluyor...");

    // 1. accept()'i kır
    int fd = serverFd_;
    serverFd_ = -1;
    if (fd >= 0) { ::shutdown(fd, SHUT_RDWR); ::close(fd); }

    if (listenerThread_.joinable()) listenerThread_.join();
    LOG("[Server] Listener bitti.");

    // 2. Client handler'ları lock DIŞINDA durdur
    //    Önce map'i taşı, sonra lock'u bırak, sonra stop() çağır.
    //    Böylece unregisterClient'ın mtx_ almaya çalışması deadlock yaratmaz.
    std::vector<std::unique_ptr<ClientHandler>> snapshot;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& [k, v] : clients_) snapshot.push_back(std::move(v));
        clients_.clear();
    }
    for (auto& h : snapshot) h->stop();

    LOG("[Server] Durduruldu.");
}

bool Server::sendTo(int clientFd, const std::string& msg)
{
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = clients_.find(clientFd);
    if (it == clients_.end()) return false;
    return it->second->send(msg);
}

void Server::listenerLoop()
{
    while (running_) {
        sockaddr_in ca{}; socklen_t len = sizeof(ca);
        int cfd = ::accept(serverFd_, (sockaddr*)&ca, &len);
        if (cfd < 0) {
            if (running_) LOG("[Server] accept hatası: ", std::strerror(errno));
            break;
        }

        LOG("[Server] Bağlantı: fd=", cfd, " ip=", ::inet_ntoa(ca.sin_addr));

        // Callback'leri oluştur
        auto onMsg = [this](int fd, const std::string& msg) {
            if (onMessage_) onMessage_(fd, msg);
        };

        // onDisconnect: sadece map'ten siler, stop() ÇAĞIRMAZ
        auto onDisc = [this](int fd) {
            if (onDisconnect_) onDisconnect_(fd);
            removeClient(fd);
        };

        auto handler = std::make_unique<ClientHandler>(cfd, onMsg, onDisc);
        handler->start();

        {
            std::lock_guard<std::mutex> lk(mtx_);
            clients_.emplace(cfd, std::move(handler));
        }

        if (onConnect_) onConnect_(cfd);
    }
    LOG("[Server] Listener loop çıktı.");
}

void Server::removeClient(int fd)
{
    // Bu fonksiyon readLoop thread'inden çağrılır.
    // Sadece map'ten siler — ClientHandler::stop() ÇAĞIRMAZ.
    // (stop() join yapar, kendi thread'ini join edemez)
    std::lock_guard<std::mutex> lk(mtx_);
    clients_.erase(fd);
    LOG("[Server] fd=", fd, " map'ten silindi.");
}
