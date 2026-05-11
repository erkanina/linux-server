#include "ClientHandler.h"
#include "Logger.h"
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

ClientHandler::ClientHandler(int fd, MessageCallback onMsg, DisconnectCallback onDisc)
    : fd_(fd), onMessage_(std::move(onMsg)), onDisconnect_(std::move(onDisc))
{}

ClientHandler::~ClientHandler()
{
    // Destructor her zaman Server::stop() tarafından çağrılır.
    // stop() zaten join() yaptı, thread bitti — burada sadece soketi garantiye al.
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

void ClientHandler::start()
{
    running_ = true;
    thread_  = std::thread(&ClientHandler::readLoop, this);
}

void ClientHandler::stop()
{
    running_ = false;
    // Soketi kapat → recv() hemen döner
    if (fd_ >= 0) {
        ::shutdown(fd_, SHUT_RDWR);
        ::close(fd_);
        fd_ = -1;
    }
    if (thread_.joinable())
        thread_.join();
}

bool ClientHandler::send(const std::string& msg) const
{
    if (fd_ < 0) return false;
    return ::send(fd_, msg.c_str(), msg.size(), MSG_NOSIGNAL)
           == static_cast<ssize_t>(msg.size());
}

void ClientHandler::readLoop()
{
    char buf[BUF];
    int  myFd = fd_;   // fd_ dışarıdan değişebilir, yerel kopyayı kullan

    while (running_) {
        ssize_t n = ::recv(myFd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (n == 0)
                LOG("[Client] fd=", myFd, " bağlantıyı kapattı.");
            else if (running_)
                LOG("[Client] fd=", myFd, " recv hatası: ", std::strerror(errno));
            break;
        }
        buf[n] = '\0';
        LOG("[Client] fd=", myFd, " mesaj: ", buf);
        if (onMessage_) onMessage_(myFd, std::string(buf, n));
    }

    running_ = false;

    // ÖNEMLİ: onDisconnect'i çağırmadan önce thread'i detach et.
    // Çünkü onDisconnect → Server::unregisterClient → erase → destructor
    // zinciri bu thread'den tetiklenir; join denenmez.
    if (thread_.joinable()) thread_.detach();

    if (onDisconnect_) onDisconnect_(myFd);
}
