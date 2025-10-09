// server.h
#ifndef SERVER_H
#define SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Запускает mesh сервер с поддержкой Signal Protocol
 * 
 * @return int 0 при успешном запуске, 1 при ошибке
 */
int start_mesh_server(void);

/**
 * @brief Останавливает mesh сервер и освобождает ресурсы
 */
void stop_mesh_server(void);

/**
 * @brief Проверяет статус работы сервера
 * 
 * @return int 1 если сервер работает, 0 если остановлен
 */
int is_server_running(void);

#ifdef __cplusplus
}
#endif

#endif // SERVER_H