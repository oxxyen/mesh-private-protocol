// src/main_c.c
#include "server.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int sig) {
    printf("\nПолучен сигнал %d. Завершение работы...\n", sig);
    shutdown_requested = 1;
}

int main() {
    // Устанавливаем обработчики сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("🚀 Запуск MESH сервера (C-версия)...\n");
    int result = start_mesh_server();
    printf("✅ Сервер завершил работу с кодом: %d\n", result);
    return result;
}