#include "Server.h"
#include "Logger.h"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// test satırı

static std::atomic<bool> g_quit{ false };
static Server*           g_server{ nullptr };

void handleSignal(int sig)
{
    if (sig == SIGPIPE) return;  // SIGPIPE'ı sessizce yut

    LOG("[Main] Signal received, shutting down...");
    g_quit = true;
    if (g_server) g_server->stop();
}

int main()
{
    // SIGPIPE: karşı taraf soketi kapatınca send() crash yapmak yerine
    // sadece -1 dönsün — ClientHandler::send() bunu handle ediyor
    std::signal(SIGPIPE,  handleSignal);
    std::signal(SIGINT,   handleSignal);
    std::signal(SIGTERM,  handleSignal);

    Server server(9000);
    g_server = &server;

    server.setOnConnect([](int fd) {
        LOG("[App] Client baglandi: fd=", fd);
    });

    server.setOnDisconnect([](int fd) {
        LOG("[App] Client ayrildi: fd=", fd);
    });

    server.setOnMessage([&server](int fd, const std::string& msg) {
        LOG("[App] Mesaj geldi | fd=", fd, " | ", msg);
        server.sendTo(fd, "Echo: " + msg);
    });

    if (!server.start()) {
        LOG("[Main] Server baslatilaamdi!");
        return 1;
    }

    LOG("[Main] Durdurmak icin Ctrl+C");

    while (!g_quit)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    return 0;
}
