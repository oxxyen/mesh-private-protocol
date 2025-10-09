// server.c
#include <signal/signal_protocol_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <sys/stat.h>
#include <signal/signal_protocol.h>
#include <signal/curve.h>
#include <signal/ratchet.h>
#include <signal/session_builder.h>
#include <signal/session_cipher.h>
#include <signal/key_helper.h>

// import server.h
#include "server.h"

#define PORT 5555
#define BUFFER_SIZE 8192
#define MAX_CLIENTS 1000
#define TOKEN_LENGTH 64
#define MAX_NICK_LENGTH 31
#define MAX_MESSAGE_LENGTH 2048
#define MAX_RATE_LIMIT 10
#define RATE_LIMIT_WINDOW 60
#define CONNECTION_TIMEOUT 300
#define EPOLL_MAX_EVENTS 100
#define SQLITE_DB_PATH "../database/mesh_db.sqlite"

// Signal Protocol контексты
signal_context *global_context = NULL;
sqlite3 *signal_db = NULL;

// Конфигурация
typedef struct {
    char db_path[256];
    int max_connections_per_ip;
    int enable_ratelimit;
} config_t;

typedef struct rate_limit {
    time_t window_start;
    int request_count;
} rate_limit_t;

typedef struct {
    int fd;
    SSL *ssl;
    char nick[MAX_NICK_LENGTH + 1];
    char token[TOKEN_LENGTH + 1];
    unsigned char session_key[32];
    int authorized;
    time_t connect_time;
    time_t last_activity;
    struct sockaddr_in addr;
    rate_limit_t rate_limit;
    
    // Signal Protocol состояния
    signal_protocol_store_context *store_context;
    session_cipher *cipher;
    uint32_t registration_id;
} client_t;

typedef struct {
    client_t *clients;
    int max_clients;
    pthread_mutex_t mutex;
    SSL_CTX *ssl_ctx;
    sqlite3 *db_conn;
    volatile sig_atomic_t running;
    config_t config;
    int epoll_fd;
} server_state_t;

server_state_t server;

// Signal Protocol хранилища
typedef struct {
    sqlite3 *db;
    uint32_t registration_id;
    ratchet_identity_key_pair *identity_key_pair;
} signal_store_data;

// Прототипы функций для Signal Protocol
int signal_store_init(const char *db_path);
void signal_store_cleanup(void);
int signal_protocol_init(void);

// Безопасные функции
#define strcpy_s(dest, dest_size, src) do { \
    strncpy((dest), (src), (dest_size)-1); \
    (dest)[(dest_size)-1] = '\0'; \
} while(0)

void log_event(const char *type, const char *message, client_t *client) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    
    printf("[%s] %s - %s (%s:%d)\n", timestamp, type, message, ip_str, ntohs(client->addr.sin_port));
}

int generate_secure_token(char *buffer, size_t len) {
    if (len < TOKEN_LENGTH + 1) return 0;
    
    unsigned char random_bytes[TOKEN_LENGTH / 2];
    if (!RAND_bytes(random_bytes, sizeof(random_bytes))) {
        return 0;
    }
    
    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        snprintf(buffer + i * 2, 3, "%02x", random_bytes[i]);
    }
    buffer[TOKEN_LENGTH] = '\0';
    return 1;
}

int validate_nickname(const char *nick) {
    size_t len = strnlen(nick, MAX_NICK_LENGTH);
    if (len < 3 || len > MAX_NICK_LENGTH) return 0;
    
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)nick[i]) && nick[i] != '_' && nick[i] != '-') 
            return 0;
    }
    return 1;
}

int validate_message(const char *message) {
    size_t len = strnlen(message, MAX_MESSAGE_LENGTH);
    return len > 0 && len <= MAX_MESSAGE_LENGTH;
}

void sanitize_input(char *str) {
    if (!str) return;
    
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '<' || *src == '>' || *src == '"' || *src == '\'' || 
            *src == '\\' || *src == ';' || *src == '&' || *src == '|') {
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int check_rate_limit(client_t *client) {
    if (!server.config.enable_ratelimit) return 1;
    
    time_t now = time(NULL);
    if (now - client->rate_limit.window_start > RATE_LIMIT_WINDOW) {
        client->rate_limit.window_start = now;
        client->rate_limit.request_count = 0;
    }
    
    if (client->rate_limit.request_count >= MAX_RATE_LIMIT) {
        return 0;
    }
    
    client->rate_limit.request_count++;
    return 1;
}

int ssl_safe_write(SSL *ssl, const char *data, size_t len) {
    size_t total = 0;
    while (total < len && server.running) {
        int n = SSL_write(ssl, data + total, len - total);
        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                usleep(10000);
                continue;
            }
            return -1;
        }
        total += (size_t)n;
    }
    return (int)total;
}

void load_config(void) {
    strcpy(server.config.db_path, SQLITE_DB_PATH);
    server.config.max_connections_per_ip = 5;
    server.config.enable_ratelimit = 1;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGPIPE, &sa, NULL);
}

void setup_ssl_context(void) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    server.ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!server.ssl_ctx) {
        fprintf(stderr, "SSL_CTX_new failed\n");
        exit(1);
    }
}

int ssl_safe_read(SSL *ssl, char *buffer, size_t max_len, int timeout_sec) {
    fd_set read_fds;
    struct timeval timeout;
    int ssl_fd = SSL_get_fd(ssl);
    
    if (ssl_fd < 0) return -1;
    
    FD_ZERO(&read_fds);
    FD_SET(ssl_fd, &read_fds);
    
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    int result = select(ssl_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (result <= 0) return 0;
    
    size_t total = 0;
    while (total < max_len - 1) {
        int n = SSL_read(ssl, buffer + total, max_len - total - 1);
        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ) {
                usleep(10000);
                continue;
            }
            break;
        }
        total += (size_t)n;
        
        if (buffer[total-1] == '\n') break;
        if (total >= max_len - 1) break;
    }
    
    buffer[total] = '\0';
    return (int)total;
}

void disconnect_client(client_t *client, const char *reason) {
    if (!client || client->fd < 0) return;
    
    pthread_mutex_lock(&server.mutex);
    
    printf("Disconnecting client %d: %s\n", client->fd, reason);
    log_event("DISCONNECT", reason, client);
    
    // Очистка Signal Protocol ресурсов
    if (client->cipher) {
        session_cipher_free(client->cipher);
        client->cipher = NULL;
    }
    
    if (client->store_context) {
        signal_protocol_store_context_destroy(client->store_context);
        client->store_context = NULL;
    }
    
    if (client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }
    
    close(client->fd);
    client->fd = -1;
    client->ssl = NULL;
    client->authorized = 0;
    memset(client->nick, 0, sizeof(client->nick));
    memset(client->token, 0, sizeof(client->token));
    memset(client->session_key, 0, sizeof(client->session_key));
    
    pthread_mutex_unlock(&server.mutex);
}

// Signal Protocol callback функции
int identity_key_store_get_identity_key_pair(signal_buffer **public_data, signal_buffer **private_data, void *user_data) {
    // TODO: загрузка данных из базы.
    signal_buffer *public_buf = signal_buffer_alloc(32);
    signal_buffer *private_buf = signal_buffer_alloc(32);
    
    if (!public_buf || !private_buf) {
        if (public_buf) signal_buffer_free(public_buf);
        if (private_buf) signal_buffer_free(private_buf);
        return SG_ERR_NOMEM;
    }
    
    // Заполняем тестовыми данными
    memset(signal_buffer_data(public_buf), 0xAA, 32);
    memset(signal_buffer_data(private_buf), 0xBB, 32);
    
    *public_data = public_buf;
    *private_data = private_buf;
    return 0;
}

int identity_key_store_get_local_registration_id(void *user_data, uint32_t *registration_id) {
    *registration_id = 1; // Базовое значение
    return 0;
}

int identity_key_store_save_identity(const char *name, size_t name_len, uint8_t *key_data, size_t key_len, void *user_data) {
    // Сохранение идентификационного ключа в базе данных
    return 0;
}

int identity_key_store_is_trusted_identity(const char *name, size_t name_len, uint8_t *key_data, size_t key_len, void *user_data) {
    // В реальной реализации здесь должна быть проверка доверия
    return 1; // Доверяем всем для примера
}

int session_store_load_session(signal_buffer **record, signal_buffer **user_record, const char *name, size_t name_len, void *user_data) {
    // Загрузка сессии из базы данных
    return 0;
}

int session_store_get_sub_device_sessions(signal_int_list **sessions, const char *name, size_t name_len, void *user_data) {
    // Получение списка подустройств
    return 0;
}

int session_store_store_session(const char *name, size_t name_len, uint8_t *record, size_t record_len, uint8_t *user_record, size_t user_record_len, void *user_data) {
    // Сохранение сессии в базе данных
    return 0;
}

int session_store_contains_session(const char *name, size_t name_len, void *user_data) {
    // Проверка наличия сессии
    return 0;
}

int session_store_delete_session(const char *name, size_t name_len, void *user_data) {
    // Удаление сессии
    return 0;
}

int session_store_delete_all_sessions(const char *name, size_t name_len, void *user_data) {
    // Удаление всех сессий
    return 0;
}

int pre_key_store_load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data) {
    // Загрузка pre-key из базы данных
    return 0;
}

int pre_key_store_store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    // Сохранение pre-key в базе данных
    return 0;
}

int pre_key_store_contains_pre_key(uint32_t pre_key_id, void *user_data) {
    // Проверка наличия pre-key
    return 0;
}

int pre_key_store_remove_pre_key(uint32_t pre_key_id, void *user_data) {
    // Удаление pre-key
    return 0;
}

int signed_pre_key_store_load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data) {
    // Загрузка signed pre-key из базы данных
    return 0;
}

int signed_pre_key_store_store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    // Сохранение signed pre-key в базе данных
    return 0;
}

int signed_pre_key_store_contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    // Проверка наличия signed pre-key
    return 0;
}

int signed_pre_key_store_remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    // Удаление signed pre-key
    return 0;
}

int signal_protocol_init(void) {
    int result = signal_context_create(&global_context, NULL);
    if (result != 0) {
        fprintf(stderr, "Failed to create signal context: %d\n", result);
        return 0;
    }
    
    // Инициализация криптографии Curve25519
    ec_key_pair *key_pair = NULL;
    result = curve_generate_key_pair(global_context, &key_pair);
    if (result != 0) {
        fprintf(stderr, "Failed to initialize crypto: %d\n", result);
        signal_context_destroy(global_context);
        global_context = NULL;
        return 0;
    }
    
    if (key_pair) {
        SIGNAL_UNREF(key_pair);
    }
    
    return 1;
}

int init_client_signal_protocol(client_t *client) {
    int result = 0;
    
    // Создание контекста хранилища
    result = signal_protocol_store_context_create(&client->store_context, global_context);
    if (result != 0) {
        fprintf(stderr, "Failed to create store context for client\n");
        return 0;
    }
    
    // Настройка хранилищ
    signal_protocol_identity_key_store identity_store = {
        .get_identity_key_pair = identity_key_store_get_identity_key_pair,
        .get_local_registration_id = identity_key_store_get_local_registration_id,
        .save_identity = identity_key_store_save_identity,
        .is_trusted_identity = identity_key_store_is_trusted_identity,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_session_store session_store = {
        .load_session_func = session_store_load_session,
        .get_sub_device_sessions_func = session_store_get_sub_device_sessions,
        .store_session_func = session_store_store_session,
        .contains_session_func = session_store_contains_session,
        .delete_session_func = session_store_delete_session,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_pre_key_store pre_key_store = {
        .load_pre_key = pre_key_store_load_pre_key,
        .store_pre_key = pre_key_store_store_pre_key,
        .contains_pre_key = pre_key_store_contains_pre_key,
        .remove_pre_key = pre_key_store_remove_pre_key,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_signed_pre_key_store signed_pre_key_store = {
        .load_signed_pre_key = signed_pre_key_store_load_signed_pre_key,
        .store_signed_pre_key = signed_pre_key_store_store_signed_pre_key,
        .contains_signed_pre_key = signed_pre_key_store_contains_signed_pre_key,
        .remove_signed_pre_key = signed_pre_key_store_remove_signed_pre_key,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    // Установка хранилищ
    result = signal_protocol_store_context_set_identity_key_store(client->store_context, &identity_store);
    if (result != 0) goto error;
    
    result = signal_protocol_store_context_set_session_store(client->store_context, &session_store);
    if (result != 0) goto error;
    
    result = signal_protocol_store_context_set_pre_key_store(client->store_context, &pre_key_store);
    if (result != 0) goto error;
    
    result = signal_protocol_store_context_set_signed_pre_key_store(client->store_context, &signed_pre_key_store);
    if (result != 0) goto error;
    
    // Генерация ключей для клиента
    ratchet_identity_key_pair *identity_key_pair = NULL;
    signal_protocol_key_helper_pre_key_list_node *pre_keys_head = NULL;
    session_signed_pre_key *signed_pre_key = NULL;
    
    result = signal_protocol_key_helper_generate_identity_key_pair(&identity_key_pair, global_context);
    if (result != 0) goto error;
    
    result = signal_protocol_key_helper_generate_registration_id(&client->registration_id, 0, global_context);
    if (result != 0) goto error;
    
    result = signal_protocol_key_helper_generate_pre_keys(&pre_keys_head, 1, 100, global_context);
    if (result != 0) goto error;
    
    uint64_t timestamp = (uint64_t)time(NULL);
    result = signal_protocol_key_helper_generate_signed_pre_key(&signed_pre_key, identity_key_pair, 5, timestamp, global_context);
    if (result != 0) goto error;
    
    // Очистка временных ключей
    if (identity_key_pair) SIGNAL_UNREF(identity_key_pair);
    if (pre_keys_head) signal_protocol_key_helper_pre_key_list_free(pre_keys_head);
    if (signed_pre_key) SIGNAL_UNREF(signed_pre_key);
    
    return 1;
    
error:
    if (client->store_context) {
        signal_protocol_store_context_destroy(client->store_context);
        client->store_context = NULL;
    }
    return 0;
}

int init_database(void) {
    int result = sqlite3_open(server.config.db_path, &server.db_conn);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(server.db_conn));
        return 0;
    }
    
    // Создание таблиц
    const char *tables[] = {
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "nick TEXT UNIQUE NOT NULL, "
        "token TEXT UNIQUE NOT NULL, "
        "public_key TEXT, "
        "identity_key_public TEXT, "
        "identity_key_private TEXT, "
        "registration_id INTEGER, "
        "ip_address TEXT, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "last_login DATETIME NULL"
        ")",
        
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "from_user TEXT NOT NULL, "
        "to_user TEXT, "
        "message_text TEXT NOT NULL, "
        "encrypted INTEGER DEFAULT 0, "
        "cipher_type INTEGER, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")",
        
        "CREATE TABLE IF NOT EXISTS ip_limits ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "ip_address TEXT NOT NULL, "
        "connection_count INTEGER DEFAULT 0, "
        "last_attempt DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")",
        
        // Таблицы для Signal Protocol
        "CREATE TABLE IF NOT EXISTS signal_sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_nick TEXT NOT NULL, "
        "device_id INTEGER, "
        "record BLOB, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")",
        
        "CREATE TABLE IF NOT EXISTS signal_pre_keys ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_nick TEXT NOT NULL, "
        "pre_key_id INTEGER, "
        "record BLOB, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")",
        
        "CREATE TABLE IF NOT EXISTS signal_signed_pre_keys ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_nick TEXT NOT NULL, "
        "signed_pre_key_id INTEGER, "
        "record BLOB, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")",
        
        "CREATE TABLE IF NOT EXISTS signal_identity_keys ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_nick TEXT NOT NULL, "
        "name TEXT NOT NULL, "
        "key_data BLOB, "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    };
    
    char *err_msg = NULL;
    for (size_t i = 0; i < sizeof(tables)/sizeof(tables[0]); i++) {
        result = sqlite3_exec(server.db_conn, tables[i], NULL, NULL, &err_msg);
        if (result != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
    
    // Создание индексов
    const char *indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_users_nick ON users(nick)",
        "CREATE INDEX IF NOT EXISTS idx_users_token ON users(token)",
        "CREATE INDEX IF NOT EXISTS idx_messages_from ON messages(from_user)",
        "CREATE INDEX IF NOT EXISTS idx_messages_to ON messages(to_user)",
        "CREATE INDEX IF NOT EXISTS idx_messages_created ON messages(created_at)",
        "CREATE INDEX IF NOT EXISTS idx_ip_limits_ip ON ip_limits(ip_address)",
        "CREATE INDEX IF NOT EXISTS idx_signal_sessions_user ON signal_sessions(user_nick)",
        "CREATE INDEX IF NOT EXISTS idx_signal_pre_keys_user ON signal_pre_keys(user_nick)",
        "CREATE INDEX IF NOT EXISTS idx_signal_signed_pre_keys_user ON signal_signed_pre_keys(user_nick)",
        "CREATE INDEX IF NOT EXISTS idx_signal_identity_keys_user ON signal_identity_keys(user_nick)"
    };
    
    for (size_t i = 0; i < sizeof(indexes)/sizeof(indexes[0]); i++) {
        result = sqlite3_exec(server.db_conn, indexes[i], NULL, NULL, &err_msg);
        if (result != SQLITE_OK) {
            fprintf(stderr, "SQL index error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
    
    return 1;
}

client_t *find_client_by_nick(const char *nick) {
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.max_clients; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized && 
            strcmp(server.clients[i].nick, nick) == 0) {
            pthread_mutex_unlock(&server.mutex);
            return &server.clients[i];
        }
    }
    pthread_mutex_unlock(&server.mutex);
    return NULL;
}

int check_ip_limit(const char *ip) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT connection_count FROM ip_limits WHERE ip_address = ? AND last_attempt > datetime('now', '-1 hour')";
    
    if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 1;
    }
    
    sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_STATIC);
    
    int result = 1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        if (count >= server.config.max_connections_per_ip) {
            result = 0;
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
}

void update_ip_limit(const char *ip, int increment) {
    sqlite3_stmt *stmt;
    
    if (increment) {
        const char *sql = "INSERT INTO ip_limits (ip_address, connection_count, last_attempt) VALUES (?, 1, datetime('now')) "
                         "ON CONFLICT(ip_address) DO UPDATE SET connection_count = connection_count + 1, last_attempt = datetime('now')";
        if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    } else {
        const char *sql = "UPDATE ip_limits SET connection_count = MAX(0, connection_count - 1) WHERE ip_address = ?";
        if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}

void broadcast_message(const char *sender_nick, const char *message) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO messages (from_user, message_text) VALUES (?, ?)";
    
    if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, sender_nick, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, message, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    char json_message[BUFFER_SIZE];
    snprintf(json_message, sizeof(json_message),
             "{\"type\":\"message\",\"from\":\"%s\",\"text\":\"%s\"}\n",
             sender_nick, message);
    
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.max_clients; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized) {
            ssl_safe_write(server.clients[i].ssl, json_message, strlen(json_message));
        }
    }
    pthread_mutex_unlock(&server.mutex);
}

void send_private_message(const char *sender_nick, const char *target_nick, const char *message) {
    client_t *target = find_client_by_nick(target_nick);
    if (!target) return;
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO messages (from_user, to_user, message_text) VALUES (?, ?, ?)";
    
    if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, sender_nick, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, target_nick, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, message, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    char json_message[BUFFER_SIZE];
    snprintf(json_message, sizeof(json_message),
             "{\"type\":\"private\",\"from\":\"%s\",\"to\":\"%s\",\"text\":\"%s\"}\n",
             sender_nick, target_nick, message);
    
    pthread_mutex_lock(&server.mutex);
    if (target->fd >= 0 && target->authorized) {
        ssl_safe_write(target->ssl, json_message, strlen(json_message));
    }
    pthread_mutex_unlock(&server.mutex);
}

void *client_handler(void *arg) {
    client_t *client = (client_t*)arg;
    char buffer[BUFFER_SIZE];
    char ip_str[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    
    if (!check_ip_limit(ip_str)) {
        const char *msg = "Too many connections from your IP address\n";
        ssl_safe_write(client->ssl, msg, strlen(msg));
        disconnect_client(client, "IP limit exceeded");
        return NULL;
    }
    
    update_ip_limit(ip_str, 1);
    
    // Инициализация Signal Protocol для клиента
    if (!init_client_signal_protocol(client)) {
        fprintf(stderr, "Failed to initialize Signal Protocol for client\n");
        disconnect_client(client, "Signal Protocol init failed");
        return NULL;
    }
    
    log_event("CONNECT", "New connection", client);
    
    const char *welcome = "Welcome to Veil Secure Chat!\n"
                         "Commands: /register <nick>, /login <nick> <token>, "
                         "/msg <user> <message>, /online, /help\n";
    ssl_safe_write(client->ssl, welcome, strlen(welcome));
    
    while (server.running && client->fd >= 0) {
        int bytes_read = ssl_safe_read(client->ssl, buffer, sizeof(buffer), 5);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) continue;
            break;
        }
        
        client->last_activity = time(NULL);
        
        if (!check_rate_limit(client)) {
            const char *msg = "Rate limit exceeded. Please wait.\n";
            ssl_safe_write(client->ssl, msg, strlen(msg));
            continue;
        }
        
        sanitize_input(buffer);
        
        if (strlen(buffer) == 0 || buffer[0] == '\n') continue;
        
        if (buffer[strlen(buffer)-1] == '\n') {
            buffer[strlen(buffer)-1] = '\0';
        }
        
        if (!client->authorized) {
            if (strncmp(buffer, "/register", 9) == 0) {
                char nick[32];
                if (sscanf(buffer, "/register %31s", nick) == 1 && validate_nickname(nick)) {
                    char token[TOKEN_LENGTH + 1];
                    if (generate_secure_token(token, sizeof(token))) {
                        sqlite3_stmt *stmt;
                        const char *sql = "INSERT INTO users (nick, token, ip_address, registration_id) VALUES (?, ?, ?, ?)";
                        
                        if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
                            sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt, 2, token, -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt, 3, ip_str, -1, SQLITE_STATIC);
                            sqlite3_bind_int(stmt, 4, client->registration_id);
                            
                            if (sqlite3_step(stmt) == SQLITE_DONE) {
                                char response[256];
                                snprintf(response, sizeof(response),
                                         "{\"type\":\"register\",\"status\":\"success\",\"nick\":\"%s\",\"token\":\"%s\"}\n",
                                         nick, token);
                                ssl_safe_write(client->ssl, response, strlen(response));
                                log_event("REGISTER", nick, client);
                            } else {
                                const char *error_msg = "{\"type\":\"register\",\"status\":\"error\",\"message\":\"Nickname already exists\"}\n";
                                ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
                            }
                            sqlite3_finalize(stmt);
                        }
                    }
                } else {
                    const char *error_msg = "{\"type\":\"register\",\"status\":\"error\",\"message\":\"Invalid nickname\"}\n";
                    ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
                }
            }
            else if (strncmp(buffer, "/login", 6) == 0) {
                char nick[32], token[TOKEN_LENGTH + 1];
                if (sscanf(buffer, "/login %31s %63s", nick, token) == 2) {
                    sqlite3_stmt *stmt;
                    const char *sql = "SELECT token FROM users WHERE nick = ?";
                    
                    if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
                        sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
                        
                        if (sqlite3_step(stmt) == SQLITE_ROW) {
                            const char *db_token = (const char*)sqlite3_column_text(stmt, 0);
                            if (db_token && strcmp(db_token, token) == 0) {
                                strcpy_s(client->nick, sizeof(client->nick), nick);
                                strcpy_s(client->token, sizeof(client->token), token);
                                client->authorized = 1;
                                
                                char response[128];
                                snprintf(response, sizeof(response),
                                         "{\"type\":\"login\",\"status\":\"success\",\"nick\":\"%s\"}\n", nick);
                                ssl_safe_write(client->ssl, response, strlen(response));
                                
                                // Обновление времени последнего входа
                                sqlite3_finalize(stmt);
                                const char *update_sql = "UPDATE users SET last_login = datetime('now') WHERE nick = ?";
                                if (sqlite3_prepare_v2(server.db_conn, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
                                    sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
                                    sqlite3_step(stmt);
                                    sqlite3_finalize(stmt);
                                }
                                
                                log_event("LOGIN", nick, client);
                                
                                char welcome_msg[256];
                                snprintf(welcome_msg, sizeof(welcome_msg),
                                         "User %s joined the chat\n", nick);
                                broadcast_message("System", welcome_msg);
                            } else {
                                const char *error_msg = "{\"type\":\"login\",\"status\":\"error\",\"message\":\"Invalid credentials\"}\n";
                                ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
                            }
                        } else {
                            const char *error_msg = "{\"type\":\"login\",\"status\":\"error\",\"message\":\"Invalid credentials\"}\n";
                            ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
                        }
                        sqlite3_finalize(stmt);
                    }
                }
            }
        } else {
            if (strcmp(buffer, "/online") == 0) {
                char online_list[BUFFER_SIZE] = "{\"type\":\"online\",\"users\":[";
                pthread_mutex_lock(&server.mutex);
                int first = 1;
                for (int i = 0; i < server.max_clients; i++) {
                    if (server.clients[i].fd >= 0 && server.clients[i].authorized) {
                        if (!first) {
                            strncat(online_list, ",", sizeof(online_list) - strlen(online_list) - 1);
                        }
                        strncat(online_list, "\"", sizeof(online_list) - strlen(online_list) - 1);
                        strncat(online_list, server.clients[i].nick, sizeof(online_list) - strlen(online_list) - 1);
                        strncat(online_list, "\"", sizeof(online_list) - strlen(online_list) - 1);
                        first = 0;
                    }
                }
                pthread_mutex_unlock(&server.mutex);
                strncat(online_list, "]}\n", sizeof(online_list) - strlen(online_list) - 1);
                ssl_safe_write(client->ssl, online_list, strlen(online_list));
            }
            else if (strncmp(buffer, "/msg", 4) == 0) {
                char target[32], message[MAX_MESSAGE_LENGTH];
                if (sscanf(buffer, "/msg %31s %2047[^\n]", target, message) >= 2) {
                    if (validate_message(message)) {
                        send_private_message(client->nick, target, message);
                        char response[128];
                        snprintf(response, sizeof(response),
                                 "{\"type\":\"private_sent\",\"to\":\"%s\",\"status\":\"success\"}\n", target);
                        ssl_safe_write(client->ssl, response, strlen(response));
                    }
                }
            }
            else if (strcmp(buffer, "/help") == 0) {
                const char *help = 
                    "{\"type\":\"help\",\"commands\":[\n"
                    "  \"/online - Show online users\",\n"
                    "  \"/msg <user> <message> - Send private message\",\n"
                    "  \"/help - Show this help\"\n"
                    "]}\n";
                ssl_safe_write(client->ssl, help, strlen(help));
            }
            else {
                if (validate_message(buffer)) {
                    broadcast_message(client->nick, buffer);
                }
            }
        }
    }
    
    if (client->authorized) {
        char leave_msg[256];
        snprintf(leave_msg, sizeof(leave_msg), "User %s left the chat\n", client->nick);
        broadcast_message("System", leave_msg);
    }
    
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    update_ip_limit(ip_str, 0);
    
    disconnect_client(client, "Client disconnected");
    return NULL;
}

int setup_epoll(void) {
    server.epoll_fd = epoll_create1(0);
    if (server.epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    return 0;
}

void *monitor_thread(void *arg) {
    (void)arg;
    
    while (server.running) {
        sleep(30);
        
        time_t now = time(NULL);
        pthread_mutex_lock(&server.mutex);
        
        for (int i = 0; i < server.max_clients; i++) {
            if (server.clients[i].fd >= 0) {
                if (now - server.clients[i].last_activity > CONNECTION_TIMEOUT) {
                    printf("Disconnecting inactive client %s\n", server.clients[i].nick);
                    disconnect_client(&server.clients[i], "Inactive timeout");
                }
            }
        }
        
        pthread_mutex_unlock(&server.mutex);
        
        // Проверка соединения с базой данных SQLite
        if (server.db_conn) {
            sqlite3_stmt *stmt;
            if (sqlite3_prepare_v2(server.db_conn, "SELECT 1", -1, &stmt, NULL) != SQLITE_OK) {
                fprintf(stderr, "Database connection lost, reconnecting...\n");
                sqlite3_close(server.db_conn);
                init_database();
            } else {
                sqlite3_finalize(stmt);
            }
        }
    }
    return NULL;
}

void cleanup_resources(void) {
    printf("Cleaning up resources...\n");
    server.running = 0;
    
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.max_clients; i++) {
        if (server.clients[i].fd >= 0) {
            disconnect_client(&server.clients[i], "Server shutdown");
        }
    }
    pthread_mutex_unlock(&server.mutex);
    
    if (server.epoll_fd >= 0) close(server.epoll_fd);
    if (server.ssl_ctx) SSL_CTX_free(server.ssl_ctx);
    if (server.db_conn) sqlite3_close(server.db_conn);
    if (server.clients) free(server.clients);
    
    // Очистка Signal Protocol
    if (global_context) {
        signal_context_destroy(global_context);
        global_context = NULL;
    }
    
    EVP_cleanup();
    ERR_free_strings();
}

int start_mesh_server(void) {
    printf("Starting Veil Secure Chat Server with Signal Protocol...\n");
    
    memset(&server, 0, sizeof(server));
    server.running = 1;
    server.max_clients = MAX_CLIENTS;
    
    load_config();
    setup_signal_handlers();
    
    if (pthread_mutex_init(&server.mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return 1;
    }
    
    server.clients = calloc(MAX_CLIENTS, sizeof(client_t));
    if (!server.clients) {
        fprintf(stderr, "Memory allocation failed\n");
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server.clients[i].fd = -1;
    }
    
    setup_ssl_context();
    
    // Инициализация Signal Protocol
    if (!signal_protocol_init()) {
        fprintf(stderr, "Signal Protocol initialization failed\n");
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    printf("Signal Protocol initialized successfully\n");
    
    if (!init_database()) {
        fprintf(stderr, "Database initialization failed\n");
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    printf("SQLite database connected successfully\n");
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    
    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    if (listen(server_fd, 1000) < 0) {
        perror("listen");
        close(server_fd);
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    printf("Server listening on port %d\n", PORT);
    
    pthread_t monitor_tid;
    pthread_create(&monitor_tid, NULL, monitor_thread, NULL);
    pthread_detach(monitor_tid);
    
    if (setup_epoll() == -1) {
        close(server_fd);
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        cleanup_resources();
        pthread_mutex_destroy(&server.mutex);
        return 1;
    }
    
    struct epoll_event events[EPOLL_MAX_EVENTS];
    
    while (server.running) {
        int nfds = epoll_wait(server.epoll_fd, events, EPOLL_MAX_EVENTS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                    continue;
                }
                
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                
                pthread_mutex_lock(&server.mutex);
                int client_index = -1;
                for (int j = 0; j < server.max_clients; j++) {
                    if (server.clients[j].fd < 0) {
                        client_index = j;
                        break;
                    }
                }
                
                if (client_index == -1) {
                    close(client_fd);
                    pthread_mutex_unlock(&server.mutex);
                    continue;
                }
                
                SSL *client_ssl = SSL_new(server.ssl_ctx);
                SSL_set_fd(client_ssl, client_fd);
                
                if (SSL_accept(client_ssl) <= 0) {
                    fprintf(stderr, "SSL_accept failed. Error details:\n");
                    int ssl_error = SSL_get_error(client_ssl, -1);
                    fprintf(stderr, "SSL error code: %d\n", ssl_error);
                    
                    SSL_free(client_ssl);
                    close(client_fd);
                    pthread_mutex_unlock(&server.mutex);
                    continue;
                }
                
                server.clients[client_index].fd = client_fd;
                server.clients[client_index].ssl = client_ssl;
                server.clients[client_index].addr = client_addr;
                server.clients[client_index].connect_time = time(NULL);
                server.clients[client_index].last_activity = time(NULL);
                server.clients[client_index].authorized = 0;
                memset(server.clients[client_index].nick, 0, sizeof(server.clients[client_index].nick));
                memset(server.clients[client_index].token, 0, sizeof(server.clients[client_index].token));
                server.clients[client_index].rate_limit.window_start = time(NULL);
                server.clients[client_index].rate_limit.request_count = 0;
                
                // Инициализация Signal Protocol полей
                server.clients[client_index].store_context = NULL;
                server.clients[client_index].cipher = NULL;
                server.clients[client_index].registration_id = 0;
                
                pthread_mutex_unlock(&server.mutex);
                
                pthread_t client_tid;
                pthread_create(&client_tid, NULL, client_handler, &server.clients[client_index]);
                pthread_detach(client_tid);
            }
        }
    }
    
    close(server_fd);
    cleanup_resources();
    pthread_mutex_destroy(&server.mutex);
    
    printf("Server shutdown completed\n");
    return 0;
}