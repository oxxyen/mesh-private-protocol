// MeshServer.hpp
#ifndef MESH_SERVER_HPP
#define MESH_SERVER_HPP

#include <thread>
#include <stdexcept>
#include <iostream>

extern "C" {
    int start_mesh_server(void);
    void stop_mesh_server(void);
    int is_server_running(void);
}

class MeshServer {
public:
    MeshServer() = default;
    ~MeshServer() { stop(); }

    void start();
    void stop();
    bool isRunning() const { return is_server_running() != 0; }

private:
    std::thread server_thread;
    bool started = false;
};

inline void MeshServer::start() {
    if (started) {
        throw std::runtime_error("Server already started");
    }
    if (isRunning()) {
        throw std::runtime_error("Server is already running (externally)");
    }

    started = true;
    server_thread = std::thread([]() {
        int result = start_mesh_server();
        if (result != 0) {
            std::cerr << "⚠️ Mesh server exited with error code: " << result << std::endl;
        }
    });
}

inline void MeshServer::stop() {
    if (!started) return;

    stop_mesh_server(); // ← Это устанавливает server.running = 0 в C

    if (server_thread.joinable()) {
        server_thread.join();
    }
    started = false;
}

#endif // MESH_SERVER_HPP