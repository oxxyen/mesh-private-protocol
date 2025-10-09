// src/MeshServer.hpp
#ifndef MESH_SERVER_HPP
#define MESH_SERVER_HPP

#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>

// Объявление C-функции на глобальном уровне
extern "C" int start_mesh_server(void);

class MeshServer {
public:
    MeshServer();
    ~MeshServer();

    void start();
    void stop();
    bool isRunning() const { return running; }

private:
    std::thread server_thread;
    std::atomic<bool> running{false};
};

inline MeshServer::MeshServer() = default;

inline MeshServer::~MeshServer() {
    stop();
}

inline void MeshServer::start() {
    if (running.exchange(true)) {
        throw std::runtime_error("Server already running");
    }

    server_thread = std::thread([]() {
        int result = start_mesh_server();
        if (result != 0) {
            std::cerr << "Mesh server exited with error code: " << result << std::endl;
        }
    });
}

inline void MeshServer::stop() {
    if (running.exchange(false)) {
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
}

#endif // MESH_SERVER_HPP