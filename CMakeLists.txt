cmake_minimum_required(VERSION 3.14)
project(MeshServer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

find_package(Threads REQUIRED)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# Общие исходники (без main!)
set(COMMON_SOURCES
    src/server.c
)

# === C-версия: mesh_server ===
add_executable(mesh_server
    src/main_c.c        # ← ТОЧКА ВХОДА ДЛЯ C
    ${COMMON_SOURCES}
)
target_link_libraries(mesh_server
    sqlite3
    ssl
    crypto
    m                   # ← обязательно для libsignal
    pthread
    signal-protocol-c
    Threads::Threads
)
target_compile_definitions(mesh_server PRIVATE HAVE_SIGNAL_PROTOCOL)

# === C++ обёртка: mesh_server_cpp ===
add_executable(mesh_server_cpp
    src/main.cpp        # ← ТОЧКА ВХОДА ДЛЯ C++
    src/MeshServer.hpp
    ${COMMON_SOURCES}
)
target_link_libraries(mesh_server_cpp
    sqlite3
    ssl
    crypto
    m                   # ← тоже нужно!
    pthread
    signal-protocol-c
    Threads::Threads
)
target_compile_definitions(mesh_server_cpp PRIVATE HAVE_SIGNAL_PROTOCOL)