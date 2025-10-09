cmake_minimum_required(VERSION 3.14)
project(MeshServer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

# Найти зависимости
find_package(Threads REQUIRED)

# Пути к Signal Protocol
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# Исходники
set(SOURCES
    src/server.c
    src/main.cpp
)

# Цель
add_executable(mesh_server ${SOURCES})

# Линковка
target_link_libraries(mesh_server
    sqlite3
    ssl
    crypto
    pthread
    signal-protocol-c
    Threads::Threads
)

# Определяем макрос, чтобы включить Signal Protocol
target_compile_definitions(mesh_server PRIVATE HAVE_SIGNAL_PROTOCOL)