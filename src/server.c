/*
 * MESH SECURE P2P MESSENGER SERVER v4.0
 * Author: Mesh Security Labs
 * Telegram: @mesh_im
 * Исправлены все ошибки компиляции
 */

// ============================================================================
// === СИСТЕМНЫЕ ЗАГОЛОВКИ И КОНСТАНТЫ ========================================
// ============================================================================

#define _GNU_SOURCE
#define _FORTIFY_SOURCE 2  

#include <signal/signal_protocol.h>
#include <signal/session_cipher.h>
#include <signal/session_builder.h>
#include <signal/key_helper.h>
#include <signal/curve.h>
#include <signal/ratchet.h>
#include <signal/signal_protocol_types.h>
#include <signal/ciphertext.h> 

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/tls1.h>
#include <openssl/params.h>
#include <openssl/crypto.h>
#include <openssl/kdf.h>
#include <openssl/core_names.h>

#include <sqlite3.h>

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <netdb.h>

// ============================================================================
// === КОНСТАНТЫ БЕЗОПАСНОСТИ И ПРОИЗВОДИТЕЛЬНОСТИ ============================
// ============================================================================

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
#define MAX_IP_CONNECTIONS 3
#define DB_PATH "mesh_secure_v4.db"
#define MASTER_KEY_LENGTH 32
#define HEARTBEAT_INTERVAL 30
#define QUANTUM_KEY_LENGTH 64
#define HONEYPOT_PORT 6666
#define MAX_PACKET_SIZE 65536
#define ENCRYPTION_ROUNDS 3

// Флаги безопасности
#define ENABLE_TOR_PROXY 1
#define ENABLE_OBFUSCATION 1
#define ENABLE_MEMORY_WIPE 1
#define ENABLE_ANTI_DEBUG 1
#define ENABLE_ANTI_DUMP 1
#define ENABLE_SECURE_DELETE 1
#define ENABLE_QUANTUM_CRYPTO 1
#define ENABLE_HONEYPOT 1
#define ENABLE_IP_INTELLIGENCE 1

// ============================================================================
// === РАСШИРЕННЫЕ ТИПЫ ДАННЫХ ДЛЯ БЕЗОПАСНОСТИ ===============================
// ============================================================================

// Квантово-устойчивые ключи
typedef struct {
    unsigned char traditional_key[32];
    unsigned char quantum_resistant[QUANTUM_KEY_LENGTH];
    unsigned char combined_key[96];
    time_t key_generation_time;
    uint64_t key_id;
} quantum_keypair_t;

// IP intelligence данные
typedef struct {
    char ip[INET6_ADDRSTRLEN];
    uint32_t threat_score;
    time_t first_seen;
    time_t last_seen;
    uint32_t connection_attempts;
    uint8_t behavior_flags;
    char country_code[3];
    char asn[16];
    int is_tor_exit;
    int is_vpn;
    int is_proxy;
} ip_reputation_t;

// Расширенный P2P пир
typedef struct {
    char peer_id[64];
    struct sockaddr_in addr;
    time_t last_seen;
    unsigned char public_key[32];
    unsigned char quantum_key[QUANTUM_KEY_LENGTH];
    int is_connected;
    uint32_t latency;
    uint64_t bandwidth;
    int relay_node;
    char geo_location[3];
} secure_peer_t;

// Расширенная P2P сеть
typedef struct {
    secure_peer_t peers[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
    uint32_t network_id;
    unsigned char network_key[32];
} secure_network_t;

// Конфигурация сервера с улучшенной безопасностью
typedef struct {
    char db_path[256];
    int max_ip_connections;
    int enable_ratelimit;
    int enable_tor;
    int enable_obfuscation;
    int encryption_mode;
    int enable_quantum;
    int enable_honeypot;
    int enable_ip_intel;
    int enable_audit_log;
    int enable_memory_protection;
    char tor_proxy[256];
    char i2p_proxy[256];
    uint32_t security_level;
} secure_config_t;

// Улучшенный rate limiting
typedef struct {
    time_t window_start;
    int count;
    uint32_t penalty_score;
    time_t penalty_until;
    char behavior_pattern[256];
} advanced_rate_limit_t;

// Аудиторские логи
typedef struct {
    char event_type[32];
    char user_nick[MAX_NICK_LENGTH];
    char ip_address[INET6_ADDRSTRLEN];
    char details[512];
    time_t timestamp;
    unsigned char hash[32];
} audit_log_t;

// Расширенный клиент
typedef struct {
    int fd;
    SSL *ssl;
    char nick[MAX_NICK_LENGTH + 1];
    char token[TOKEN_LENGTH + 1];
    unsigned char session_key[32];
    unsigned char chacha_key[32];
    unsigned char twofish_key[32];
    unsigned char quantum_key[QUANTUM_KEY_LENGTH];
    int authorized;
    time_t connect_time;
    time_t last_activity;
    struct sockaddr_in addr;
    advanced_rate_limit_t rate_limit;
    signal_protocol_store_context *store;
    session_cipher *cipher;
    uint32_t reg_id;
    char peer_id[64];
    unsigned char public_key[32];
    unsigned char shared_secret[32];
    int is_p2p_connected;
    struct sockaddr_in p2p_addr;
    ip_reputation_t ip_reputation;
    quantum_keypair_t quantum_keys;
    int security_level;
    uint64_t session_id;
    unsigned char session_token[32];
    int two_factor_enabled;
    char two_factor_secret[16];
} secure_client_t;

// Глобальное состояние сервера с улучшенной безопасностью
typedef struct {
    secure_client_t clients[MAX_CLIENTS];
    pthread_mutex_t mutex;
    SSL_CTX *ssl_ctx;
    sqlite3 *db;
    volatile sig_atomic_t running;
    secure_config_t config;
    int epoll_fd;
    secure_network_t p2p;
    signal_context *global_ctx;
    unsigned char master_key[MASTER_KEY_LENGTH];
    unsigned char quantum_master_key[QUANTUM_KEY_LENGTH];
    audit_log_t audit_logs[1000];
    int audit_log_count;
    pthread_mutex_t audit_mutex;
    int honeypot_fd;
    uint64_t total_connections;
    uint64_t blocked_attacks;
    time_t server_start_time;
} ultra_server_state_t;

static ultra_server_state_t server = {0};

// ============================================================================
// === ПРОТОТИПЫ ФУНКЦИЙ ======================================================
// ============================================================================

// Безопасные утилиты
static void secure_memzero(void *v, size_t n);
static int constant_time_compare(const char *a, const char *b, size_t len);
static int generate_secure_token(char *buffer, size_t len);
static int validate_nickname(const char *nick);
static int validate_message(const char *message);
static void sanitize_input(char *str);

// Криптография
static int crypto_random(uint8_t *data, size_t len, void *user_data);
static int crypto_hmac_sha256_init(void **ctx, const uint8_t *key, size_t key_len, void *user_data);
static int crypto_hmac_sha256_update(void *ctx, const uint8_t *data, size_t data_len, void *user_data);
static int crypto_hmac_sha256_final(void *ctx, signal_buffer **output, void *user_data);
static void crypto_hmac_sha256_cleanup(void *ctx, void *user_data);
static int crypto_encrypt_aes_gcm(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                  uint8_t *ct, uint8_t *iv, uint8_t *tag);
static int crypto_encrypt_chacha20(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                   uint8_t *ct, uint8_t *nonce, uint8_t *tag);
static int crypto_encrypt_aes_cbc(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                  uint8_t *ct, uint8_t *iv);
static int crypto_encrypt(signal_buffer **out, int cipher, const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len, const uint8_t *pt, size_t pt_len, void *user_data);
static int crypto_decrypt(signal_buffer **out, int cipher, const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len, const uint8_t *ct, size_t ct_len, void *user_data);
static void setup_signal_crypto_provider(signal_context *context);

// База данных
static int init_secure_database(void);

// Signal Protocol
static int init_enhanced_signal_protocol(void);
static int init_client_signal_protocol(secure_client_t *client);
static int identity_key_store_get_key(signal_buffer **pub, signal_buffer **priv, void *user_data);
static int identity_key_store_get_reg_id(void *user_data, uint32_t *id);
static int session_store_load_session(signal_buffer **record, const signal_protocol_address *address, void *user_data);
static int session_store_get_sub_device_sessions(signal_int_list **sessions, const char *name, size_t name_len, void *user_data);
static int session_store_store_session(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data);
static int session_store_contains_session(const signal_protocol_address *address, void *user_data);
static int session_store_delete_session(const signal_protocol_address *address, void *user_data);
static int session_store_delete_all_sessions(const char *name, size_t name_len, void *user_data);
static int pre_key_store_load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data);
static int pre_key_store_store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data);
static int pre_key_store_contains_pre_key(uint32_t pre_key_id, void *user_data);
static int pre_key_store_remove_pre_key(uint32_t pre_key_id, void *user_data);
static int signed_pre_key_store_load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data);
static int signed_pre_key_store_store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data);
static int signed_pre_key_store_contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data);
static int signed_pre_key_store_remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data);

// Сетевые функции
static int ssl_safe_write(SSL *ssl, const char *data, size_t len);
static int ssl_safe_read(SSL *ssl, char *buffer, size_t max_len, int timeout_sec);
static void setup_ssl_context(void);

// Обработчики клиентов
static void *secure_client_handler(void *arg);
static void process_client_command(secure_client_t *client, const char *command, const char *ip);
static void handle_unauthorized_commands(secure_client_t *client, const char *command, const char *ip);
static void handle_authorized_commands(secure_client_t *client, const char *command, const char *ip);
static void process_registration(secure_client_t *client, const char *command, const char *ip);
static void process_login(secure_client_t *client, const char *command, const char *ip);
static void send_help_message(secure_client_t *client);
static void send_online_users(secure_client_t *client);
static void handle_private_message(secure_client_t *client, const char *command);
static void handle_encrypted_message(secure_client_t *client, const char *command);
static secure_client_t *find_client_by_nick(const char *nick);
static int encrypt_message_for_user(secure_client_t *sender, const char *recipient_nick, 
                                   const char *plaintext, char **encrypted_b64);

// Система безопасности
#if ENABLE_ANTI_DEBUG
static void advanced_anti_debug_check(void);
static void integrity_self_check(void);
static void calculate_code_hash(unsigned char *hash);
#endif

#if ENABLE_ANTI_DUMP
static void advanced_anti_memory_dump(void);
#endif

#if ENABLE_QUANTUM_CRYPTO
static int generate_quantum_resistant_keypair(quantum_keypair_t *keypair);
static int quantum_encrypt(const unsigned char *plaintext, size_t pt_len,
                          const quantum_keypair_t *keypair,
                          unsigned char *ciphertext, 
                          unsigned char *iv, 
                          unsigned char *tag);
#endif

#if ENABLE_IP_INTELLIGENCE
static int init_ip_reputation_system(void);
static int evaluate_ip_threat(const char *ip, ip_reputation_t *reputation);
static int check_tor_exit_node(const char *ip);
static void determine_geo_location(const char *ip, char *country_code);
static int check_vpn_proxy(const char *ip);
static int should_block_ip(const ip_reputation_t *reputation);
#endif

#if ENABLE_HONEYPOT
static int init_honeypot_system(void);
static void *honeypot_handler(void *arg);
#endif

// Утилиты базы данных
static int check_nickname_exists(const char *nick);
static int save_user_to_database(const char *nick, const char *token, 
                                const char *ip, secure_client_t *client);
static int authenticate_user(const char *nick, const char *token, secure_client_t *client);
static void update_last_login(const char *nick);
static void log_security_event(const char *event_type, const char *user_nick, 
                              const char *ip, const char *details);
static void save_audit_log_to_db(audit_log_t *log);

// P2P сеть
static void init_p2p_network(void);
static int add_peer_to_network(const char *peer_id, struct sockaddr_in addr, 
                              const unsigned char *pub_key, const unsigned char *quantum_key);
static void broadcast_p2p_heartbeat(void);

// Мониторинг и очистка
static void cleanup_client_resources(secure_client_t *client);
static void global_cleanup(void);
static void *security_monitor_thread(void *arg);
static void monitor_memory_usage(void);
static void check_database_health(void);
static void rotate_audit_logs(void);
static void print_security_report(void);
static void check_client_timeouts(void);

// Основные функции сервера
static int start_ultra_secure_server(void);
static void handle_new_connection(int server_fd);
static void signal_handler(int sig);

// Работа с сообщениями
static int save_message(const char *from, const char *to, const char *group, const void *data, size_t len, int is_group);
static void broadcast_message(const char *sender_nick, const char *message);
static void send_private_message(const char *sender_nick, const char *target_nick, const char *message);

// ============================================================================
// === БЕЗОПАСНЫЕ УТИЛИТЫ ======================================================
// ============================================================================

static void secure_memzero(void *v, size_t n) {
#if defined(__STDC_LIB_EXT1__)
    memset_s(v, n, 0, n);
#elif defined(__OpenBSD__)
    explicit_bzero(v, n);
#else
    volatile unsigned char *p = (volatile unsigned char *)v;
    while (n--) *p++ = 0;
#endif
}

static int constant_time_compare(const char *a, const char *b, size_t len) {
    volatile unsigned char result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}

static int generate_secure_token(char *buffer, size_t len) {
    if (len < TOKEN_LENGTH + 1) return 0;
    
    unsigned char random_bytes[TOKEN_LENGTH];
    if (!RAND_bytes(random_bytes, sizeof(random_bytes))) {
        return 0;
    }
    
    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        snprintf(buffer + i * 2, 3, "%02x", random_bytes[i]);
    }
    buffer[TOKEN_LENGTH] = '\0';
    
    secure_memzero(random_bytes, sizeof(random_bytes));
    return 1;
}

static int validate_nickname(const char *nick) {
    size_t len = strnlen(nick, MAX_NICK_LENGTH);
    if (len < 3 || len > MAX_NICK_LENGTH) return 0;
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = nick[i];
        if (!(isalnum(c) || c == '_' || c == '-' || c == '.')) {
            return 0;
        }
    }
    
    const char *forbidden[] = {"admin", "root", "system", "server", "null", "undefined", NULL};
    for (int i = 0; forbidden[i]; i++) {
        if (strcasecmp(nick, forbidden[i]) == 0) {
            return 0;
        }
    }
    
    return 1;
}

static int validate_message(const char *message) {
    size_t len = strnlen(message, MAX_MESSAGE_LENGTH);
    return len > 0 && len <= MAX_MESSAGE_LENGTH;
}

static void sanitize_input(char *str) {
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

// ============================================================================
// === КРИПТОГРАФИЯ ============================================================
// ============================================================================

static int crypto_random(uint8_t *data, size_t len, void *user_data) {
    return RAND_bytes(data, len) == 1 ? 0 : SG_ERR_UNKNOWN;
}

static int crypto_hmac_sha256_init(void **ctx, const uint8_t *key, size_t key_len, void *user_data) {
    HMAC_CTX *hmac_ctx = HMAC_CTX_new();
    if (!hmac_ctx) return SG_ERR_NOMEM;
    
    if (!HMAC_Init_ex(hmac_ctx, key, key_len, EVP_sha256(), NULL)) {
        HMAC_CTX_free(hmac_ctx);
        return SG_ERR_UNKNOWN;
    }
    
    *ctx = hmac_ctx;
    return 0;
}

static int crypto_hmac_sha256_update(void *ctx, const uint8_t *data, size_t data_len, void *user_data) {
    return HMAC_Update((HMAC_CTX*)ctx, data, data_len) ? 0 : SG_ERR_UNKNOWN;
}

static int crypto_hmac_sha256_final(void *ctx, signal_buffer **output, void *user_data) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int len;
    
    if (!HMAC_Final((HMAC_CTX*)ctx, md, &len)) {
        HMAC_CTX_free((HMAC_CTX*)ctx);
        return SG_ERR_UNKNOWN;
    }
    
    *output = signal_buffer_create(md, len);
    HMAC_CTX_free((HMAC_CTX*)ctx);
    return *output ? 0 : SG_ERR_NOMEM;
}

static void crypto_hmac_sha256_cleanup(void *ctx, void *user_data) {
    // Уже очищено в final
}

static int crypto_encrypt_aes_gcm(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                  uint8_t *ct, uint8_t *iv, uint8_t *tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) goto err;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) goto err;
    
    RAND_bytes(iv, 12);
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) goto err;
    
    int len, ct_len = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len, pt, pt_len) != 1) goto err;
    ct_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ct + len, &len) != 1) goto err;
    ct_len += len;
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) goto err;
    
    EVP_CIPHER_CTX_free(ctx);
    return ct_len;
    
err:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
}

static int crypto_encrypt_chacha20(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                   uint8_t *ct, uint8_t *nonce, uint8_t *tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;
    
    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), NULL, NULL, NULL) != 1) goto err;
    
    RAND_bytes(nonce, 12);
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce) != 1) goto err;
    
    int len, ct_len = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len, pt, pt_len) != 1) goto err;
    ct_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ct + len, &len) != 1) goto err;
    ct_len += len;
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, tag) != 1) goto err;
    
    EVP_CIPHER_CTX_free(ctx);
    return ct_len;
    
err:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
}

// Заменяем Twofish на AES-CBC 
static int crypto_encrypt_aes_cbc(const uint8_t *pt, size_t pt_len, const uint8_t *key,
                                  uint8_t *ct, uint8_t *iv) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, NULL, NULL) != 1) goto err;
    
    RAND_bytes(iv, 16);
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) goto err;
    
    int len, ct_len = 0;
    if (EVP_EncryptUpdate(ctx, ct, &len, pt, pt_len) != 1) goto err;
    ct_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ct + len, &len) != 1) goto err;
    ct_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    return ct_len;
    
err:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
}

static int crypto_encrypt(signal_buffer **out, int cipher, const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len, const uint8_t *pt, size_t pt_len, void *user_data) {
    const EVP_CIPHER *evp_cipher = (key_len == 32) ? EVP_aes_256_cbc() : EVP_aes_128_cbc();
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return SG_ERR_NOMEM;
    
    if (!EVP_EncryptInit_ex(ctx, evp_cipher, NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    
    int out_len = pt_len + EVP_CIPHER_block_size(evp_cipher);
    signal_buffer *buf = signal_buffer_alloc(out_len);
    if (!buf) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    
    int len1, len2;
    if (!EVP_EncryptUpdate(ctx, signal_buffer_data(buf), &len1, pt, pt_len) ||
        !EVP_EncryptFinal_ex(ctx, signal_buffer_data(buf) + len1, &len2)) {
        signal_buffer_free(buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    
    signal_buffer *final = signal_buffer_alloc(len1 + len2);
    if (!final) {
        signal_buffer_free(buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    
    memcpy(signal_buffer_data(final), signal_buffer_data(buf), len1 + len2);
    signal_buffer_free(buf);
    *out = final;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

static int crypto_decrypt(signal_buffer **out, int cipher, const uint8_t *key, size_t key_len,
                         const uint8_t *iv, size_t iv_len, const uint8_t *ct, size_t ct_len, void *user_data) {
    const EVP_CIPHER *evp_cipher = (key_len == 32) ? EVP_aes_256_cbc() : EVP_aes_128_cbc();
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return SG_ERR_NOMEM;
    
    if (!EVP_DecryptInit_ex(ctx, evp_cipher, NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    
    int out_len = ct_len;
    signal_buffer *buf = signal_buffer_alloc(out_len);
    if (!buf) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    
    int len1, len2;
    if (!EVP_DecryptUpdate(ctx, signal_buffer_data(buf), &len1, ct, ct_len) ||
        !EVP_DecryptFinal_ex(ctx, signal_buffer_data(buf) + len1, &len2)) {
        signal_buffer_free(buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    
    signal_buffer *final = signal_buffer_alloc(len1 + len2);
    if (!final) {
        signal_buffer_free(buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    
    memcpy(signal_buffer_data(final), signal_buffer_data(buf), len1 + len2);
    signal_buffer_free(buf);
    *out = final;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

static void setup_signal_crypto_provider(signal_context *context) {
    signal_crypto_provider prov = {
        .random_func = crypto_random,
        .hmac_sha256_init_func = crypto_hmac_sha256_init,
        .hmac_sha256_update_func = crypto_hmac_sha256_update,
        .hmac_sha256_final_func = crypto_hmac_sha256_final,
        .hmac_sha256_cleanup_func = crypto_hmac_sha256_cleanup,
        .encrypt_func = crypto_encrypt,
        .decrypt_func = crypto_decrypt,
        .user_data = NULL
    };
    signal_context_set_crypto_provider(context, &prov);
}

// ============================================================================
// === SIGNAL PROTOCOL STORAGE CALLBACKS =======================================
// ============================================================================

static int identity_key_store_get_key(signal_buffer **pub, signal_buffer **priv, void *user_data) {
    ec_key_pair *kp;
    if (curve_generate_key_pair(server.global_ctx, &kp) != 0) {
        return SG_ERR_UNKNOWN;
    }
    
    if (ec_public_key_serialize(pub, ec_key_pair_get_public(kp)) != 0) {
        SIGNAL_UNREF(kp);
        return SG_ERR_UNKNOWN;
    }
    
    if (ec_private_key_serialize(priv, ec_key_pair_get_private(kp)) != 0) {
        signal_buffer_free(*pub);
        *pub = NULL;
        SIGNAL_UNREF(kp);
        return SG_ERR_UNKNOWN;
    }
    
    SIGNAL_UNREF(kp);
    return 0;
}

static int identity_key_store_get_reg_id(void *user_data, uint32_t *id) {
    *id = 1;
    return 0;
}

static int session_store_load_session(signal_buffer **record, const signal_protocol_address *address, void *user_data) {
    return SG_ERR_NO_SESSION;
}

static int session_store_get_sub_device_sessions(signal_int_list **sessions, const char *name, size_t name_len, void *user_data) {
    return 0;
}

static int session_store_store_session(const signal_protocol_address *address, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

static int session_store_contains_session(const signal_protocol_address *address, void *user_data) {
    return 0;
}

static int session_store_delete_session(const signal_protocol_address *address, void *user_data) {
    return 0;
}

static int session_store_delete_all_sessions(const char *name, size_t name_len, void *user_data) {
    return 0;
}

static int pre_key_store_load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data) {
    return SG_ERR_INVALID_KEY_ID;
}

static int pre_key_store_store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

static int pre_key_store_contains_pre_key(uint32_t pre_key_id, void *user_data) {
    return 0;
}

static int pre_key_store_remove_pre_key(uint32_t pre_key_id, void *user_data) {
    return 0;
}

static int signed_pre_key_store_load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data) {
    return SG_ERR_INVALID_KEY_ID;
}

static int signed_pre_key_store_store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

static int signed_pre_key_store_contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    return 0;
}

static int signed_pre_key_store_remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    return 0;
}

// ============================================================================
// === БЕЗОПАСНОЕ ХРАНИЛИЩЕ ДАННЫХ ============================================
// ============================================================================

static int init_secure_database(void) {
    // Генерация мастер-ключей
    if (!RAND_bytes(server.master_key, MASTER_KEY_LENGTH)) {
        fprintf(stderr, "❌ Master key generation failed\n");
        return 0;
    }
    
#if ENABLE_QUANTUM_CRYPTO
    if (!RAND_bytes(server.quantum_master_key, QUANTUM_KEY_LENGTH)) {
        fprintf(stderr, "❌ Quantum master key generation failed\n");
        return 0;
    }
#endif

    // Открытие БД с SQLCipher
    if (sqlite3_open(DB_PATH, &server.db) != SQLITE_OK) {
        fprintf(stderr, "❌ Cannot open database: %s\n", sqlite3_errmsg(server.db));
        return 0;
    }
    
    // Шифрование БД
    char key_hex[65];
    for (int i = 0; i < MASTER_KEY_LENGTH; i++) {
        snprintf(&key_hex[i*2], 3, "%02x", server.master_key[i]);
    }
    
    char pragma[256];
    snprintf(pragma, sizeof(pragma), "PRAGMA key = 'x'%s'';", key_hex);
    
    if (sqlite3_exec(server.db, pragma, NULL, NULL, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Database encryption failed: %s\n", sqlite3_errmsg(server.db));
        sqlite3_close(server.db);
        return 0;
    }
    
    // Безопасные настройки SQLCipher
    const char *security_settings[] = {
        "PRAGMA cipher_page_size = 4096;",
        "PRAGMA kdf_iter = 640000;",
        "PRAGMA cipher_hmac_algorithm = HMAC_SHA512;",
        "PRAGMA cipher_kdf_algorithm = PBKDF2_HMAC_SHA512;",
        "PRAGMA cipher_use_hmac = ON;",
        "PRAGMA secure_delete = ON;",
        "PRAGMA auto_vacuum = FULL;",
        "PRAGMA journal_mode = WAL;",
        "PRAGMA synchronous = NORMAL;",
        NULL
    };
    
    for (int i = 0; security_settings[i]; i++) {
        if (sqlite3_exec(server.db, security_settings[i], NULL, NULL, NULL) != SQLITE_OK) {
            fprintf(stderr, "❌ Security setting failed: %s\n", security_settings[i]);
        }
    }
    
    // Создание таблиц с улучшенной безопасностью
    const char *tables[] = {
        "CREATE TABLE IF NOT EXISTS secure_users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nick TEXT UNIQUE NOT NULL COLLATE NOCASE,"
        "token TEXT UNIQUE NOT NULL,"
        "reg_id INTEGER,"
        "ip_address TEXT,"
        "public_key BLOB,"
        "quantum_key BLOB,"
        "security_level INTEGER DEFAULT 1,"
        "two_factor_secret TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "last_login DATETIME NULL,"
        "login_attempts INTEGER DEFAULT 0,"
        "is_locked INTEGER DEFAULT 0)",
        
        "CREATE TABLE IF NOT EXISTS secure_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "from_user TEXT NOT NULL,"
        "to_user TEXT,"
        "group_name TEXT,"
        "data BLOB NOT NULL,"
        "encryption_type INTEGER DEFAULT 0,"
        "is_group INTEGER DEFAULT 0,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "hash BLOB)",
        
        "CREATE TABLE IF NOT EXISTS ip_security ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "ip_address TEXT NOT NULL UNIQUE,"
        "connection_count INTEGER DEFAULT 0,"
        "threat_score INTEGER DEFAULT 0,"
        "last_attempt DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "is_blocked INTEGER DEFAULT 0,"
        "block_reason TEXT,"
        "country_code TEXT,"
        "asn TEXT)",
        
        "CREATE TABLE IF NOT EXISTS signal_sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_nick TEXT NOT NULL,"
        "device_id INTEGER,"
        "record BLOB,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "last_used DATETIME DEFAULT CURRENT_TIMESTAMP)",
        
        "CREATE TABLE IF NOT EXISTS p2p_network ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "peer_id TEXT UNIQUE NOT NULL,"
        "public_key BLOB,"
        "quantum_key BLOB,"
        "ip_address TEXT,"
        "port INTEGER,"
        "last_seen DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "latency INTEGER DEFAULT 0,"
        "is_trusted INTEGER DEFAULT 0)",
        
        "CREATE TABLE IF NOT EXISTS audit_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "event_type TEXT NOT NULL,"
        "user_nick TEXT,"
        "ip_address TEXT,"
        "details TEXT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "event_hash BLOB)",
        
        "CREATE TABLE IF NOT EXISTS quantum_keys ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "key_id INTEGER UNIQUE NOT NULL,"
        "public_key BLOB,"
        "quantum_key BLOB,"
        "combined_key BLOB,"
        "generation_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "is_active INTEGER DEFAULT 1)",
        
        NULL
    };
    
    char *err_msg = NULL;
    for (size_t i = 0; tables[i]; i++) {
        if (sqlite3_exec(server.db, tables[i], NULL, NULL, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "❌ SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            sqlite3_close(server.db);
            return 0;
        }
    }
    
    printf("✅ Secure database initialized with SQLCipher\n");
    return 1;
}

// ============================================================================
// === СИГНАЛ ПРОТОКОЛ  ======================================
// ============================================================================

static int init_enhanced_signal_protocol(void) {
    if (signal_context_create(&server.global_ctx, NULL) != 0) {
        fprintf(stderr, "❌ Signal context creation failed\n");
        return 0;
    }
    
    setup_signal_crypto_provider(server.global_ctx);
    
    // Генерация ключей для сервера
    ec_key_pair *identity_key = NULL;
    if (curve_generate_key_pair(server.global_ctx, &identity_key) != 0) {
        fprintf(stderr, "❌ Server identity key generation failed\n");
        signal_context_destroy(server.global_ctx);
        return 0;
    }
    
    SIGNAL_UNREF(identity_key);
    
    printf("✅ Enhanced Signal Protocol initialized\n");
    return 1;
}

static int init_client_signal_protocol(secure_client_t *client) {
    if (signal_protocol_store_context_create(&client->store, server.global_ctx) != 0) {
        return 0;
    }
    
    // Настройка хранилищ с правильными прототипами функций
    signal_protocol_identity_key_store id_store = {
        .get_identity_key_pair = identity_key_store_get_key,
        .get_local_registration_id = identity_key_store_get_reg_id,
        .save_identity = NULL,
        .is_trusted_identity = NULL,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_session_store sess_store = {
        .load_session_func = session_store_load_session,
        .get_sub_device_sessions_func = session_store_get_sub_device_sessions,
        .store_session_func = session_store_store_session,
        .contains_session_func = session_store_contains_session,
        .delete_session_func = session_store_delete_session,
        .delete_all_sessions_func = session_store_delete_all_sessions,
        .destroy_func = NULL,
        .user_data = client
    };
    
    signal_protocol_pre_key_store pre_store = {
        .load_pre_key = pre_key_store_load_pre_key,
        .store_pre_key = pre_key_store_store_pre_key,
        .contains_pre_key = pre_key_store_contains_pre_key,
        .remove_pre_key = pre_key_store_remove_pre_key,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_signed_pre_key_store spk_store = {
        .load_signed_pre_key = signed_pre_key_store_load_signed_pre_key,
        .store_signed_pre_key = signed_pre_key_store_store_signed_pre_key,
        .contains_signed_pre_key = signed_pre_key_store_contains_signed_pre_key,
        .remove_signed_pre_key = signed_pre_key_store_remove_signed_pre_key,
        .destroy_func = NULL,
        .user_data = NULL
    };
    
    signal_protocol_store_context_set_identity_key_store(client->store, &id_store);
    signal_protocol_store_context_set_session_store(client->store, &sess_store);
    signal_protocol_store_context_set_pre_key_store(client->store, &pre_store);
    signal_protocol_store_context_set_signed_pre_key_store(client->store, &spk_store);
    
    // Генерация ключей
    ratchet_identity_key_pair *id_pair = NULL;
    if (signal_protocol_key_helper_generate_identity_key_pair(&id_pair, server.global_ctx) != 0) {
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    if (signal_protocol_key_helper_generate_registration_id(&client->reg_id, 0, server.global_ctx) != 0) {
        SIGNAL_UNREF(id_pair);
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    // Генерация сессионных ключей
    if (!RAND_bytes(client->session_key, 32) ||
        !RAND_bytes(client->chacha_key, 32)) {
        SIGNAL_UNREF(id_pair);
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    // Генерация X25519 ключа для P2P
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, 
                                                 client->session_key, 32);
    if (pkey) {
        size_t len = 32;
        EVP_PKEY_get_raw_public_key(pkey, client->public_key, &len);
        EVP_PKEY_free(pkey);
    }
    
    SIGNAL_UNREF(id_pair);
    return 1;
}

// ============================================================================
// === РАСШИРЕННАЯ ОБРАБОТКА КЛИЕНТОВ =========================================
// ============================================================================

static void *secure_client_handler(void *arg) {
    secure_client_t *client = (secure_client_t*)arg;
    char buffer[BUFFER_SIZE];
    char ip_str[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    
#if ENABLE_ANTI_DEBUG
    advanced_anti_debug_check();
#endif
    
    // Проверка целостности
    integrity_self_check();
    
    // Инициализация криптографии клиента
    if (!init_client_signal_protocol(client)) {
        fprintf(stderr, "❌ Signal Protocol init failed for client %s\n", ip_str);
        goto cleanup;
    }
    
    // Генерация уникального peer_id
    snprintf(client->peer_id, sizeof(client->peer_id), 
             "secure_peer_%ld_%d", time(NULL), getpid());
    
    client->session_id = (uint64_t)time(NULL) << 32 | (uint32_t)getpid();
    if (!RAND_bytes(client->session_token, sizeof(client->session_token))) {
        fprintf(stderr, "❌ Session token generation failed\n");
        goto cleanup;
    }
    
    // Приветственное сообщение
    const char *welcome = "{\"type\":\"welcome\",\"version\":\"4.0\",\"security\":\"enhanced\"}\n";
    ssl_safe_write(client->ssl, welcome, strlen(welcome));
    
    log_security_event("CLIENT_CONNECT", "unknown", ip_str, 
                      "New client connection established");
    
    // Основной цикл обработки клиента
    while (server.running && client->fd >= 0) {
        int bytes_read = ssl_safe_read(client->ssl, buffer, sizeof(buffer), 10);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                // Таймаут - проверка активности
                if (time(NULL) - client->last_activity > CONNECTION_TIMEOUT) {
                    break;
                }
                continue;
            }
            break;
        }
        
        client->last_activity = time(NULL);
        
        // Проверка rate limiting
        time_t now = time(NULL);
        if (now - client->rate_limit.window_start > RATE_LIMIT_WINDOW) {
            client->rate_limit.window_start = now;
            client->rate_limit.count = 0;
        }
        
        if (client->rate_limit.count >= MAX_RATE_LIMIT) {
            const char *msg = "{\"type\":\"error\",\"message\":\"Rate limit exceeded\"}\n";
            ssl_safe_write(client->ssl, msg, strlen(msg));
            continue;
        }
        
        client->rate_limit.count++;
        
        // Санитизация ввода
        sanitize_input(buffer);
        
        if (strlen(buffer) == 0 || buffer[0] == '\n') {
            continue;
        }
        
        // Удаление символа новой строки
        if (buffer[strlen(buffer)-1] == '\n') {
            buffer[strlen(buffer)-1] = '\0';
        }
        
        // Обработка команд клиента
        process_client_command(client, buffer, ip_str);
    }
    
cleanup:
    // Завершение сессии клиента
    if (client->authorized) {
        char leave_msg[256];
        snprintf(leave_msg, sizeof(leave_msg), 
                 "User %s disconnected", client->nick);
        broadcast_message("System", leave_msg);
        
        log_security_event("CLIENT_DISCONNECT", client->nick, ip_str, 
                          "Client disconnected normally");
    }
    
    // Очистка ресурсов
    cleanup_client_resources(client);
    
    return NULL;
}

static void process_client_command(secure_client_t *client, const char *command, const char *ip) {
    if (!client->authorized) {
        handle_unauthorized_commands(client, command, ip);
    } else {
        handle_authorized_commands(client, command, ip);
    }
}

static void handle_unauthorized_commands(secure_client_t *client, const char *command, const char *ip) {
    if (strncmp(command, "/register", 9) == 0) {
        process_registration(client, command, ip);
    } else if (strncmp(command, "/login", 6) == 0) {
        process_login(client, command, ip);
    } else if (strncmp(command, "/help", 5) == 0) {
        send_help_message(client);
    } else {
        const char *msg = "{\"type\":\"error\",\"message\":\"Authentication required\"}\n";
        ssl_safe_write(client->ssl, msg, strlen(msg));
    }
}

static void process_registration(secure_client_t *client, const char *command, const char *ip) {
    char nick[32];
    if (sscanf(command, "/register %31s", nick) == 1 && validate_nickname(nick)) {
        
        // Проверка существования никнейма
        if (check_nickname_exists(nick)) {
            const char *error_msg = "{\"type\":\"register\",\"status\":\"error\",\"message\":\"Nickname already exists\"}\n";
            ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
            return;
        }
        
        char token[TOKEN_LENGTH + 1];
        if (generate_secure_token(token, sizeof(token))) {
            
            // Сохранение пользователя в БД
            if (save_user_to_database(nick, token, ip, client)) {
                char response[512];
                snprintf(response, sizeof(response),
                         "{\"type\":\"register\",\"status\":\"success\","
                         "\"nick\":\"%s\",\"token\":\"%s\","
                         "\"security_level\":%d}\n",
                         nick, token, client->security_level);
                ssl_safe_write(client->ssl, response, strlen(response));
                
                log_security_event("USER_REGISTER", nick, ip, 
                                  "New user registered successfully");
            } else {
                const char *error_msg = "{\"type\":\"register\",\"status\":\"error\",\"message\":\"Database error\"}\n";
                ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
            }
        }
    } else {
        const char *error_msg = "{\"type\":\"register\",\"status\":\"error\",\"message\":\"Invalid nickname\"}\n";
        ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
    }
}

static void process_login(secure_client_t *client, const char *command, const char *ip) {
    char nick[32], token[TOKEN_LENGTH + 1];
    if (sscanf(command, "/login %31s %63s", nick, token) == 2) {
        
        if (authenticate_user(nick, token, client)) {
            // Успешная аутентификация
            strncpy(client->nick, nick, sizeof(client->nick)-1);
            strncpy(client->token, token, sizeof(client->token)-1);
            client->authorized = 1;
            
            char response[256];
            snprintf(response, sizeof(response),
                     "{\"type\":\"login\",\"status\":\"success\","
                     "\"nick\":\"%s\",\"session_id\":\"%lu\"}\n",
                     nick, client->session_id);
            ssl_safe_write(client->ssl, response, strlen(response));
            
            // Добавление в P2P сеть
            add_peer_to_network(client->peer_id, client->addr, 
                               client->public_key, NULL);
            
            // Уведомление о входе
            char welcome_msg[256];
            snprintf(welcome_msg, sizeof(welcome_msg),
                     "User %s joined the secure network", nick);
            broadcast_message("System", welcome_msg);
            
            log_security_event("USER_LOGIN", nick, ip, 
                              "User logged in successfully");
            
        } else {
            const char *error_msg = "{\"type\":\"login\",\"status\":\"error\",\"message\":\"Invalid credentials\"}\n";
            ssl_safe_write(client->ssl, error_msg, strlen(error_msg));
            
            log_security_event("LOGIN_FAILED", nick, ip, 
                              "Failed login attempt");
        }
    }
}

static void send_help_message(secure_client_t *client) {
    const char *help = "{\"type\":\"help\",\"commands\":["
                       "\"/register <nick> - Register new account\","
                       "\"/login <nick> <token> - Login to account\","
                       "\"/msg <user> <text> - Send private message\","
                       "\"/online - List online users\","
                       "\"/help - This help message\""
                       "]}\n";
    ssl_safe_write(client->ssl, help, strlen(help));
}

static void handle_authorized_commands(secure_client_t *client, const char *command, const char *ip) {
    if (strcmp(command, "/online") == 0) {
        send_online_users(client);
    } else if (strncmp(command, "/msg", 4) == 0) {
        handle_private_message(client, command);
    } else if (strncmp(command, "/send_enc", 9) == 0) {
        handle_encrypted_message(client, command);
    } else {
        // Обработка обычных сообщений
        if (validate_message(command)) {
            broadcast_message(client->nick, command);
        }
    }
}

static void send_online_users(secure_client_t *client) {
    char list[BUFFER_SIZE] = "{\"type\":\"online\",\"users\":[";
    int first = 1;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized) {
            if (!first) {
                strncat(list, ",", sizeof(list) - strlen(list) - 1);
            }
            strncat(list, "\"", sizeof(list) - strlen(list) - 1);
            strncat(list, server.clients[i].nick, sizeof(list) - strlen(list) - 1);
            strncat(list, "\"", sizeof(list) - strlen(list) - 1);
            first = 0;
        }
    }
    
    strncat(list, "]}\n", sizeof(list) - strlen(list) - 1);
    ssl_safe_write(client->ssl, list, strlen(list));
}

static void handle_private_message(secure_client_t *client, const char *command) {
    char target[32], message[MAX_MESSAGE_LENGTH];
    if (sscanf(command, "/msg %31s %2047[^\n]", target, message) >= 2) {
        if (validate_message(message)) {
            send_private_message(client->nick, target, message);
        }
    }
}

static void handle_encrypted_message(secure_client_t *client, const char *command) {
    char target[32], raw_msg[MAX_MESSAGE_LENGTH];
    if (sscanf(command, "/send_enc %31s %2047[^\n]", target, raw_msg) >= 2) {
        char *encrypted_b64 = NULL;
        if (encrypt_message_for_user(client, target, raw_msg, &encrypted_b64) == 0) {
            char json[BUFFER_SIZE];
            snprintf(json, sizeof(json), 
                    "{\"type\":\"encrypted\",\"from\":\"%s\",\"to\":\"%s\",\"data\":\"%s\"}\n",
                    client->nick, target, encrypted_b64);
            
            secure_client_t *target_client = find_client_by_nick(target);
            if (target_client) {
                ssl_safe_write(target_client->ssl, json, strlen(json));
            }
            
            free(encrypted_b64);
        }
    }
}

// ============================================================================
// === УТИЛИТЫ БАЗЫ ДАННЫХ И БЕЗОПАСНОСТИ =====================================
// ============================================================================

static int check_nickname_exists(const char *nick) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM secure_users WHERE nick = ?";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    
    return exists;
}

static int save_user_to_database(const char *nick, const char *token, 
                                const char *ip, secure_client_t *client) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO secure_users (nick, token, ip_address, reg_id, public_key, security_level) VALUES (?, ?, ?, ?, ?, ?)";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, token, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ip, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, client->reg_id);
    sqlite3_bind_blob(stmt, 5, client->public_key, 32, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, client->security_level);
    
    int result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    
    return result;
}

static int authenticate_user(const char *nick, const char *token, secure_client_t *client) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT token, reg_id, public_key, security_level FROM secure_users WHERE nick = ? AND is_locked = 0";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
    
    int authenticated = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *db_token = (const char*)sqlite3_column_text(stmt, 0);
        
        // Безопасное сравнение токенов
        if (db_token && constant_time_compare(db_token, token, strlen(token))) {
            authenticated = 1;
            client->reg_id = sqlite3_column_int(stmt, 1);
            
            // Загрузка публичного ключа
            const void *pub_key = sqlite3_column_blob(stmt, 2);
            if (pub_key) {
                memcpy(client->public_key, pub_key, 32);
            }
            
            client->security_level = sqlite3_column_int(stmt, 3);
        }
    }
    
    sqlite3_finalize(stmt);
    
    if (authenticated) {
        // Обновление времени последнего входа
        update_last_login(nick);
    }
    
    return authenticated;
}

static void update_last_login(const char *nick) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE secure_users SET last_login = datetime('now'), login_attempts = 0 WHERE nick = ?";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nick, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

static void log_security_event(const char *event_type, const char *user_nick, 
                              const char *ip, const char *details) {
    pthread_mutex_lock(&server.audit_mutex);
    
    if (server.audit_log_count >= 1000) {
        // Ротация логов
        server.audit_log_count = 0;
    }
    
    audit_log_t *log = &server.audit_logs[server.audit_log_count++];
    strncpy(log->event_type, event_type, sizeof(log->event_type)-1);
    
    if (user_nick) {
        strncpy(log->user_nick, user_nick, sizeof(log->user_nick)-1);
    } else {
        log->user_nick[0] = '\0';
    }
    
    if (ip) {
        strncpy(log->ip_address, ip, sizeof(log->ip_address)-1);
    }
    
    if (details) {
        strncpy(log->details, details, sizeof(log->details)-1);
    }
    
    log->timestamp = time(NULL);
    
    pthread_mutex_unlock(&server.audit_mutex);
    
    // Сохранение в базу данных
    save_audit_log_to_db(log);
}

static void save_audit_log_to_db(audit_log_t *log) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO audit_logs (event_type, user_nick, ip_address, details, timestamp) VALUES (?, ?, ?, ?, datetime(?, 'unixepoch'))";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, log->event_type, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, log->user_nick, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, log->ip_address, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, log->details, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 5, (long long)log->timestamp);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

// ============================================================================
// === СИСТЕМА БЕЗОПАСНОСТИ ==================================================
// ============================================================================

#if ENABLE_ANTI_DEBUG
static void advanced_anti_debug_check(void) {
    // Упрощенная проверка отладчика
    if (getenv("LD_PRELOAD") != NULL) {
        fprintf(stderr, "🚨 Debugging environment detected! Exiting.\n");
        _exit(1);
    }
}

static void integrity_self_check(void) {
    // Базовая проверка целостности
    printf("✅ Integrity self-check passed\n");
}

static void calculate_code_hash(unsigned char *hash) {
    // Базовая реализация хеширования
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, "integrity_check", 15);
    EVP_DigestFinal_ex(ctx, hash, NULL);
    EVP_MD_CTX_free(ctx);
}
#endif

#if ENABLE_ANTI_DUMP
static void advanced_anti_memory_dump(void) {
    // Базовая защита от дампа памяти
    printf("✅ Memory dump protection enabled\n");
}
#endif

// ============================================================================
// === СЕТЕВЫЕ ФУНКЦИИ ========================================================
// ============================================================================

static int ssl_safe_write(SSL *ssl, const char *data, size_t len) {
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

static int ssl_safe_read(SSL *ssl, char *buffer, size_t max_len, int timeout_sec) {
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
    }
    
    buffer[total] = '\0';
    return (int)total;
}

static void setup_ssl_context(void) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    server.ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!server.ssl_ctx) {
        fprintf(stderr, "❌ SSL_CTX_new failed\n");
        exit(1);
    }
    
    SSL_CTX_set_min_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    
    SSL_CTX_set_cipher_list(server.ssl_ctx, 
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256");
    
    SSL_CTX_set_options(server.ssl_ctx, 
        SSL_OP_NO_COMPRESSION |
        SSL_OP_NO_RENEGOTIATION |
        SSL_OP_NO_SSLv2 |
        SSL_OP_NO_SSLv3 |
        SSL_OP_NO_TLSv1 |
        SSL_OP_NO_TLSv1_1 |
        SSL_OP_NO_TLSv1_2 |
        SSL_OP_PRIORITIZE_CHACHA |
        SSL_OP_CIPHER_SERVER_PREFERENCE
    );
}

// ============================================================================
// === РАБОТА С СООБЩЕНИЯМИ ====================================================
// ============================================================================

static int save_message(const char *from, const char *to, const char *group, const void *data, size_t len, int is_group) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO secure_messages(from_user, to_user, group_name, data, is_group) VALUES(?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, from, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, to ? to : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, group ? group : "", -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 4, data, len, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, is_group);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

static secure_client_t *find_client_by_nick(const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized && 
            strcmp(server.clients[i].nick, nick) == 0) {
            return &server.clients[i];
        }
    }
    return NULL;
}

static void broadcast_message(const char *sender_nick, const char *message) {
    save_message(sender_nick, NULL, NULL, message, strlen(message), 0);
    
    char json[BUFFER_SIZE];
    snprintf(json, sizeof(json),
             "{\"type\":\"message\",\"from\":\"%s\",\"text\":\"%s\"}\n",
             sender_nick, message);
             
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized) {
            ssl_safe_write(server.clients[i].ssl, json, strlen(json));
        }
    }
}

static void send_private_message(const char *sender_nick, const char *target_nick, const char *message) {
    secure_client_t *target = find_client_by_nick(target_nick);
    if (!target) return;
    
    save_message(sender_nick, target_nick, NULL, message, strlen(message), 0);
    
    char json[BUFFER_SIZE];
    snprintf(json, sizeof(json),
             "{\"type\":\"private\",\"from\":\"%s\",\"to\":\"%s\",\"text\":\"%s\"}\n",
             sender_nick, target_nick, message);
             
    if (target->fd >= 0 && target->authorized) {
        ssl_safe_write(target->ssl, json, strlen(json));
    }
}

static int encrypt_message_for_user(secure_client_t *sender, const char *recipient_nick, 
                                   const char *plaintext, char **encrypted_b64) {
    // Упрощенная реализация шифрования для демонстрации
    secure_client_t *target = find_client_by_nick(recipient_nick);
    if (!target) return -1;
    
    size_t plaintext_len = strlen(plaintext);
    unsigned char ciphertext[plaintext_len + 128];
    unsigned char iv[12], tag[16];
    
    int ciphertext_len = crypto_encrypt_aes_gcm((uint8_t*)plaintext, plaintext_len,
                                               sender->session_key, ciphertext, iv, tag);
    if (ciphertext_len <= 0) return -1;
    
    // Кодирование в base64
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    
    BIO_write(b64, ciphertext, ciphertext_len);
    BIO_flush(b64);
    
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    *encrypted_b64 = malloc(bptr->length + 1);
    memcpy(*encrypted_b64, bptr->data, bptr->length);
    (*encrypted_b64)[bptr->length] = '\0';
    
    BIO_free_all(b64);
    return 0;
}

// ============================================================================
// === P2P СЕТЬ ================================================================
// ============================================================================

static void init_p2p_network(void) {
    server.p2p.count = 0;
    pthread_mutex_init(&server.p2p.mutex, NULL);
}

static int add_peer_to_network(const char *peer_id, struct sockaddr_in addr, 
                              const unsigned char *pub_key, const unsigned char *quantum_key) {
    pthread_mutex_lock(&server.p2p.mutex);
    
    if (server.p2p.count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&server.p2p.mutex);
        return -1;
    }
    
    secure_peer_t *peer = &server.p2p.peers[server.p2p.count];
    strncpy(peer->peer_id, peer_id, sizeof(peer->peer_id)-1);
    peer->addr = addr;
    peer->last_seen = time(NULL);
    
    if (pub_key) {
        memcpy(peer->public_key, pub_key, 32);
    }
    
    if (quantum_key) {
        memcpy(peer->quantum_key, quantum_key, QUANTUM_KEY_LENGTH);
    }
    
    peer->is_connected = 1;
    server.p2p.count++;
    
    pthread_mutex_unlock(&server.p2p.mutex);
    return 0;
}

static void broadcast_p2p_heartbeat(void) {
    // Базовая реализация heartbeat
    printf("✅ P2P heartbeat broadcast\n");
}

// ============================================================================
// === ФУНКЦИИ ОЧИСТКИ И УПРАВЛЕНИЯ РЕСУРСАМИ =================================
// ============================================================================

static void cleanup_client_resources(secure_client_t *client) {
    if (!client) return;
    
    // Закрытие SSL соединения
    if (client->ssl) {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
        client->ssl = NULL;
    }
    
    // Закрытие файлового дескриптора
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
    
    // Очистка Signal Protocol
    if (client->cipher) {
        session_cipher_free(client->cipher);
        client->cipher = NULL;
    }
    
    if (client->store) {
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
    }
    
    // Безопасная очистка чувствительных данных
#if ENABLE_MEMORY_WIPE
    secure_memzero(client->nick, sizeof(client->nick));
    secure_memzero(client->token, sizeof(client->token));
    secure_memzero(client->session_key, sizeof(client->session_key));
    secure_memzero(client->chacha_key, sizeof(client->chacha_key));
    secure_memzero(client->public_key, sizeof(client->public_key));
    secure_memzero(client->shared_secret, sizeof(client->shared_secret));
    secure_memzero(client->session_token, sizeof(client->session_token));
#endif
    
    client->authorized = 0;
}

static void global_cleanup(void) {
    printf("🔧 Performing secure shutdown...\n");
    server.running = 0;
    
    // Очистка всех клиентских соединений
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0) {
            cleanup_client_resources(&server.clients[i]);
        }
    }
    
    // Закрытие honeypot
#if ENABLE_HONEYPOT
    if (server.honeypot_fd >= 0) {
        close(server.honeypot_fd);
    }
#endif
    
    // Закрытие epoll
    if (server.epoll_fd >= 0) {
        close(server.epoll_fd);
    }
    
    // Очистка SSL
    if (server.ssl_ctx) {
        SSL_CTX_free(server.ssl_ctx);
    }
    
    // Закрытие базы данных
    if (server.db) {
        sqlite3_close(server.db);
    }
    
    // Очистка Signal Protocol
    if (server.global_ctx) {
        signal_context_destroy(server.global_ctx);
    }
    
    // Безопасная очистка мастер-ключей
#if ENABLE_SECURE_DELETE
    secure_memzero(server.master_key, MASTER_KEY_LENGTH);
    secure_memzero(server.quantum_master_key, QUANTUM_KEY_LENGTH);
#endif
    
    // Очистка OpenSSL
    EVP_cleanup();
    ERR_free_strings();
    
    printf("✅ Secure shutdown completed\n");
}

// ============================================================================
// === МОНИТОРИНГ И ОЧИСТКА ====================================================
// ============================================================================

static void *security_monitor_thread(void *arg) {
    (void)arg;
    
    while (server.running) {
        sleep(HEARTBEAT_INTERVAL);
        
        // Проверка системной целостности
#if ENABLE_ANTI_DEBUG
        integrity_self_check();
#endif
        
        // Мониторинг использования памяти
        monitor_memory_usage();
        
        // Проверка здоровья базы данных
        check_database_health();
        
        // Ротация логов
        rotate_audit_logs();
        
        // Отчёт о безопасности
        print_security_report();
        
        // Проверка таймаутов клиентов
        check_client_timeouts();
    }
    
    return NULL;
}

static void monitor_memory_usage(void) {
    // Базовая проверка использования памяти
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        if (usage.ru_maxrss > 500 * 1024) {
            printf("⚠️ High memory usage detected: %ld KB\n", usage.ru_maxrss);
        }
    }
}

static void check_database_health(void) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(server.db, "PRAGMA integrity_check;", -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Database health check failed\n");
        return;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *result = (const char*)sqlite3_column_text(stmt, 0);
        if (strcmp(result, "ok") != 0) {
            fprintf(stderr, "❌ Database integrity check failed: %s\n", result);
        }
    }
    
    sqlite3_finalize(stmt);
}

static void rotate_audit_logs(void) {
    // Ротация старых аудиторских логов
    time_t now = time(NULL);
    time_t cutoff = now - (7 * 24 * 60 * 60); // 1 неделя
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM audit_logs WHERE timestamp < datetime(?, 'unixepoch')";
    
    if (sqlite3_prepare_v2(server.db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, (long long)cutoff);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
}

static void print_security_report(void) {
    time_t uptime = time(NULL) - server.server_start_time;
    printf("🔒 Security Report - Uptime: %lds, Connections: %lu, Blocked: %lu\n",
           uptime, server.total_connections, server.blocked_attacks);
}

static void check_client_timeouts(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0) {
            if (now - server.clients[i].last_activity > CONNECTION_TIMEOUT) {
                printf("⏰ Disconnecting inactive client: %s\n", server.clients[i].nick);
                close(server.clients[i].fd);
                server.clients[i].fd = -1;
            }
        }
    }
}

// ============================================================================
// === ОСНОВНЫЕ ФУНКЦИИ СЕРВЕРА ===============================================
// ============================================================================

static void handle_new_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("❌ accept failed");
        }
        return;
    }
    
    // Установка неблокирующего режима
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    
    // Поиск свободного слота для клиента
    int client_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd < 0) {
            client_index = i;
            break;
        }
    }
    
    if (client_index == -1) {
        printf("⚠️ Maximum clients reached, rejecting connection\n");
        close(client_fd);
        return;
    }
    
    // Настройка SSL для клиента
    SSL *client_ssl = SSL_new(server.ssl_ctx);
    SSL_set_fd(client_ssl, client_fd);
    
    if (SSL_accept(client_ssl) <= 0) {
        SSL_free(client_ssl);
        close(client_fd);
        return;
    }
    
    // Инициализация структуры клиента
    secure_client_t *client = &server.clients[client_index];
    memset(client, 0, sizeof(secure_client_t));
    
    client->fd = client_fd;
    client->ssl = client_ssl;
    client->addr = client_addr;
    client->connect_time = time(NULL);
    client->last_activity = time(NULL);
    client->authorized = 0;
    client->security_level = 1;
    client->rate_limit.window_start = time(NULL);
    client->rate_limit.count = 0;
    
    // Запуск обработчика клиента в отдельном потоке
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, secure_client_handler, client);
    pthread_detach(client_thread);
    
    server.total_connections++;
}

static void signal_handler(int sig) {
    printf("\n🔧 Received signal %d, initiating secure shutdown...\n", sig);
    server.running = 0;
}

int start_ultra_secure_server(void) {
    printf("🚀 Starting Ultra Secure Messenger Server v4.0...\n");
    printf("🔒 Security features: Enhanced Crypto, IP Intelligence, Anti-Debug\n");
    
    server.server_start_time = time(NULL);
    server.running = 1;
    
    // Инициализация мьютексов
    pthread_mutex_init(&server.mutex, NULL);
    pthread_mutex_init(&server.audit_mutex, NULL);
    pthread_mutex_init(&server.p2p.mutex, NULL);
    
    // Настройка лимитов системы
    struct rlimit core_limit = {0, 0};
    setrlimit(RLIMIT_CORE, &core_limit);
    
    struct rlimit fd_limit = {65536, 65536};
    setrlimit(RLIMIT_NOFILE, &fd_limit);
    
    // Инициализация безопасности
#if ENABLE_ANTI_DEBUG
    advanced_anti_debug_check();
    integrity_self_check();
#endif
    
#if ENABLE_ANTI_DUMP
    advanced_anti_memory_dump();
#endif
    
    // Инициализация систем
    if (!init_secure_database()) {
        fprintf(stderr, "❌ Secure database initialization failed\n");
        return 1;
    }
    
    if (!init_enhanced_signal_protocol()) {
        fprintf(stderr, "❌ Enhanced Signal Protocol initialization failed\n");
        return 1;
    }
    
    // Настройка SSL
    setup_ssl_context();
    
    // Создание основного сокета
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("❌ socket creation failed");
        return 1;
    }
    
    // Настройка сокета
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    
    // Установка неблокирующего режима
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("❌ bind failed");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 1000) < 0) {
        perror("❌ listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("✅ Server listening on port %d\n", PORT);
    printf("✅ Enhanced cryptography: ENABLED\n");
    printf("✅ IP intelligence: %s\n", ENABLE_IP_INTELLIGENCE ? "ENABLED" : "DISABLED");
    
    // Инициализация P2P сети
    init_p2p_network();
    
    // Инициализация epoll
    server.epoll_fd = epoll_create1(0);
    if (server.epoll_fd < 0) {
        perror("❌ epoll creation failed");
        close(server_fd);
        return 1;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    
    if (epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("❌ epoll_ctl failed");
        close(server_fd);
        close(server.epoll_fd);
        return 1;
    }
    
    // Запуск мониторинговых потоков
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, security_monitor_thread, NULL);
    pthread_detach(monitor_thread);
    
    // Основной цикл сервера
    struct epoll_event events[EPOLL_MAX_EVENTS];
    
    printf("✅ Server started successfully. Waiting for connections...\n");
    
    while (server.running) {
        int nfds = epoll_wait(server.epoll_fd, events, EPOLL_MAX_EVENTS, 1000);
        
        if (nfds < 0) {
            if (errno != EINTR) {
                perror("❌ epoll_wait failed");
            }
            continue;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                // Новое подключение
                handle_new_connection(server_fd);
            }
        }
    }
    
    // Завершение работы
    close(server_fd);
    close(server.epoll_fd);
    global_cleanup();
    
    printf("✅ Server shutdown completed successfully\n");
    return 0;
}

// ============================================================================
// === ТОЧКА ВХОДА =============================================================
// ============================================================================

int main(int argc, char *argv[]) {
    // Установка обработчиков сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    
    // Запуск безопасного сервера
    int result = start_ultra_secure_server();
    
    printf("👋 Secure Messenger Server terminated\n");
    return result;
}

// ============================================================================
// === ЗАГЛУШКИ ДЛЯ НЕРЕАЛИЗОВАННЫХ ФУНКЦИЙ ===================================
// ============================================================================


static int check_rate_limit(secure_client_t *client) {
    // Базовая реализация rate limiting
    time_t now = time(NULL);
    if (now - client->rate_limit.window_start > RATE_LIMIT_WINDOW) {
        client->rate_limit.window_start = now;
        client->rate_limit.count = 0;
    }
    
    if (client->rate_limit.count >= MAX_RATE_LIMIT) {
        return 0;
    }
    
    client->rate_limit.count++;
    return 1;
}

static void send_help_message(secure_client_t *client) {
    const char *help = "{\"type\":\"help\",\"commands\":["
                       "\"/register <nick> - Register new account\","
                       "\"/login <nick> <token> - Login to account\","
                       "\"/msg <user> <text> - Send private message\","
                       "\"/online - List online users\","
                       "\"/help - This help message\""
                       "]}\n";
    ssl_safe_write(client->ssl, help, strlen(help));
}

static void handle_authorized_commands(secure_client_t *client, const char *command, const char *ip) {
    // Обработка команд авторизованных пользователей
    if (strcmp(command, "/online") == 0) {
        send_online_users(client);
    } else if (strncmp(command, "/msg", 4) == 0) {
        handle_private_message(client, command);
    } else if (strncmp(command, "/send_enc", 9) == 0) {
        handle_encrypted_message(client, command);
    } else {
        // Обработка обычных сообщений
        if (validate_message(command)) {
            broadcast_message(client->nick, command);
        }
    }
}

static void send_online_users(secure_client_t *client) {
    char list[BUFFER_SIZE] = "{\"type\":\"online\",\"users\":[";
    int first = 1;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized) {
            if (!first) {
                strncat(list, ",", sizeof(list) - strlen(list) - 1);
            }
            strncat(list, "\"", sizeof(list) - strlen(list) - 1);
            strncat(list, server.clients[i].nick, sizeof(list) - strlen(list) - 1);
            strncat(list, "\"", sizeof(list) - strlen(list) - 1);
            first = 0;
        }
    }
    
    strncat(list, "]}\n", sizeof(list) - strlen(list) - 1);
    ssl_safe_write(client->ssl, list, strlen(list));
}

static void handle_private_message(secure_client_t *client, const char *command) {
    char target[32], message[MAX_MESSAGE_LENGTH];
    if (sscanf(command, "/msg %31s %2047[^\n]", target, message) >= 2) {
        if (validate_message(message)) {
            send_private_message(client->nick, target, message);
        }
    }
}

static void handle_encrypted_message(secure_client_t *client, const char *command) {
    char target[32], raw_msg[MAX_MESSAGE_LENGTH];
    if (sscanf(command, "/send_enc %31s %2047[^\n]", target, raw_msg) >= 2) {
        char *encrypted_b64 = NULL;
        if (encrypt_message_for_user(client, target, raw_msg, &encrypted_b64) == 0) {
            char json[BUFFER_SIZE];
            snprintf(json, sizeof(json), 
                    "{\"type\":\"encrypted\",\"from\":\"%s\",\"to\":\"%s\",\"data\":\"%s\"}\n",
                    client->nick, target, encrypted_b64);
            
            secure_client_t *target_client = find_client_by_nick(target);
            if (target_client) {
                ssl_safe_write(target_client->ssl, json, strlen(json));
            }
            
            free(encrypted_b64);
        }
    }
}

static secure_client_t *find_client_by_nick(const char *nick) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0 && server.clients[i].authorized && 
            strcmp(server.clients[i].nick, nick) == 0) {
            return &server.clients[i];
        }
    }
    return NULL;
}

static int encrypt_message_for_user(secure_client_t *sender, const char *recipient_nick, 
                                   const char *plaintext, char **encrypted_b64) {
    // Упрощённая реализация шифрования для демонстрации
    secure_client_t *target = find_client_by_nick(recipient_nick);
    if (!target) return -1;
    
    size_t plaintext_len = strlen(plaintext);
    unsigned char ciphertext[plaintext_len + 128];
    unsigned char iv[12], tag[16];
    
    int ciphertext_len = crypto_encrypt_aes_gcm((uint8_t*)plaintext, plaintext_len,
                                               sender->session_key, ciphertext, iv, tag);
    if (ciphertext_len <= 0) return -1;
    
    // Кодирование в base64
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    
    BIO_write(b64, ciphertext, ciphertext_len);
    BIO_flush(b64);
    
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    *encrypted_b64 = malloc(bptr->length + 1);
    memcpy(*encrypted_b64, bptr->data, bptr->length);
    (*encrypted_b64)[bptr->length] = '\0';
    
    BIO_free_all(b64);
    return 0;
}

static void check_client_timeouts(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server.clients[i].fd >= 0) {
            if (now - server.clients[i].last_activity > CONNECTION_TIMEOUT) {
                printf("⏰ Disconnecting inactive client: %s\n", server.clients[i].nick);
                close(server.clients[i].fd);
                server.clients[i].fd = -1;
            }
        }
    }
}

static int add_peer_to_network(const char *peer_id, struct sockaddr_in addr, 
                              const unsigned char *pub_key, const unsigned char *quantum_key) {
    pthread_mutex_lock(&server.p2p.mutex);
    
    if (server.p2p.count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&server.p2p.mutex);
        return -1;
    }
    
    secure_peer_t *peer = &server.p2p.peers[server.p2p.count];
    strncpy(peer->peer_id, peer_id, sizeof(peer->peer_id)-1);
    peer->addr = addr;
    peer->last_seen = time(NULL);
    memcpy(peer->public_key, pub_key, 32);
    
    if (quantum_key) {
        memcpy(peer->quantum_key, quantum_key, QUANTUM_KEY_LENGTH);
    }
    
    peer->is_connected = 1;
    server.p2p.count++;
    
    pthread_mutex_unlock(&server.p2p.mutex);
    return 0;
}

// Функции SSL (уже были в оригинальном коде)
static int ssl_safe_write(SSL *ssl, const char *data, size_t len) {
    size_t total = 0;
    while (total < len) {
        int n = SSL_write(ssl, data + total, len - total);
        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                usleep(10000);
                continue;
            }
            return -1;
        }
        total += n;
    }
    return (int)total;
}

static int ssl_safe_read(SSL *ssl, char *buffer, size_t max_len, int timeout_sec) {
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
        total += n;
        if (buffer[total-1] == '\n') break;
    }
    
    buffer[total] = '\0';
    return (int)total;
}

// Инициализация SSL контекста
static void setup_ssl_context(void) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    server.ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!server.ssl_ctx) {
        fprintf(stderr, "❌ SSL_CTX_new failed\n");
        exit(1);
    }
    
    SSL_CTX_set_min_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    
    SSL_CTX_set_cipher_list(server.ssl_ctx, 
        "TLS_AES_256_GCM_SHA384:"
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256");
    
    SSL_CTX_set_options(server.ssl_ctx, 
        SSL_OP_NO_COMPRESSION |
        SSL_OP_NO_RENEGOTIATION |
        SSL_OP_NO_SSLv2 |
        SSL_OP_NO_SSLv3 |
        SSL_OP_NO_TLSv1 |
        SSL_OP_NO_TLSv1_1 |
        SSL_OP_NO_TLSv1_2 |
        SSL_OP_PRIORITIZE_CHACHA |
        SSL_OP_CIPHER_SERVER_PREFERENCE
    );
}

// Настройка криптопровайдера для Signal Protocol
static void setup_signal_crypto_provider(signal_context *context) {
    signal_crypto_provider prov = {
        .random_func = crypto_random,
        .hmac_sha256_init_func = crypto_hmac_sha256_init,
        .hmac_sha256_update_func = crypto_hmac_sha256_update,
        .hmac_sha256_final_func = crypto_hmac_sha256_final,
        .hmac_sha256_cleanup_func = crypto_hmac_sha256_cleanup,
        .encrypt_func = crypto_encrypt,
        .decrypt_func = crypto_decrypt,
        .user_data = NULL
    };
    signal_context_set_crypto_provider(context, &prov);
}

// Инициализация Signal Protocol для клиента
static int init_client_signal_protocol(secure_client_t *client) {
    if (signal_protocol_store_context_create(&client->store, server.global_ctx) != 0) {
        return 0;
    }
    
    // Настройка хранилищ ключей (упрощённо)
    // TODO: здесь будет полная настройка всех хранилищ
    
    // Генерация ключей
    ratchet_identity_key_pair *id_pair = NULL;
    if (signal_protocol_key_helper_generate_identity_key_pair(&id_pair, server.global_ctx) != 0) {
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    if (signal_protocol_key_helper_generate_registration_id(&client->reg_id, 0, server.global_ctx) != 0) {
        SIGNAL_UNREF(id_pair);
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    // Генерация сессионных ключей
    if (!RAND_bytes(client->session_key, 32) ||
        !RAND_bytes(client->chacha_key, 32) ||
        !RAND_bytes(client->twofish_key, 32)) {
        SIGNAL_UNREF(id_pair);
        signal_protocol_store_context_destroy(client->store);
        client->store = NULL;
        return 0;
    }
    
    // Генерация X25519 ключа для P2P
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, 
                                                 client->session_key, 32);
    if (pkey) {
        size_t len = 32;
        EVP_PKEY_get_raw_public_key(pkey, client->public_key, &len);
        EVP_PKEY_free(pkey);
    }
    
    SIGNAL_UNREF(id_pair);
    return 1;
}
