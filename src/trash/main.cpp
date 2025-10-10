// src/main.cpp
#include "MeshServer.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int sig) {
    std::cout << "\nReceived signal " << sig << ". Shutting down...\n";
    shutdown_requested = true;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        MeshServer server;
        server.start();
        std::cout << "✅ Mesh Server started via C++ wrapper\n";
        std::cout << "Press Ctrl+C to stop.\n";

        while (!shutdown_requested && server.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        server.stop();
        std::cout << "✅ Mesh Server stopped cleanly.\n";

    } catch (const std::exception &e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}