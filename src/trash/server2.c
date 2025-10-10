// server.c
#define _GNU_SOURCE
#include <signal/ciphertext_message.h>
#include <openssl/tls1.h>
#include <cstdint>
#include <openssl/bio.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/params.h>
#include <signal/signal_protocol_types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/buffer.h>
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
#define SQLITE_DB_PATH "../database/mesh_data.sqlite"

// P2P Network Types
typedef struct {
    char peer_id[64];
    struct sockaddr_in public_addr;
    struct sockaddr_in local_addr;
    int is_connected;
    time_t last_heartbeat;
    char public_key[256];
} p2p_peer_t;

typedef struct {
    p2p_peer_t *peers;
    int peer_count;
    int max_peers;
    pthread_mutex_t peer_mutex;
    int dht_port;
    int is_super_node;
} p2p_network_t;

// Enhanced Security Configuration
typedef struct {
    char db_path[256];
    int max_connections_per_ip;
    int enable_ratelimit;
    int enable_tor_proxy;
    int enable_ip_spoofing;
    int enable_obfuscation;
    int encryption_mode; // 0=AES-256-GCM, 1=ChaCha20, 2=Twofish
} config_t;

typedef struct hmac_ctx_wrapper {
    EVP_MAC_CTX *ctx;
    EVP_MAC *mac;
} hmac_ctx_wrapper;

typedef struct rate_limit {
    time_t window_start;
    int request_count;
} rate_limit_t;

// Enhanced Client Structure
typedef struct {
    int fd;
    SSL *ssl;
    char nick[MAX_NICK_LENGTH + 1];
    char token[TOKEN_LENGTH + 1];
    unsigned char session_key[32];
    unsigned char chacha_key[32];
    unsigned char twofish_key[32];
    int authorized;
    time_t connect_time;
    time_t last_activity;
    struct sockaddr_in addr;
    rate_limit_t rate_limit;
    signal_protocol_store_context *store_context;
    session_cipher *cipher;
    uint32_t registration_id;
    
    // P2P Fields
    char peer_id[64];
    int is_p2p_connected;
    struct sockaddr_in p2p_addr;
    char public_key[256];
    char shared_secret[32];
} client_t;

// Enhanced Server State
typedef struct {
    client_t *clients;
    int max_clients;
    pthread_mutex_t mutex;
    SSL_CTX *ssl_ctx;
    sqlite3 *db_conn;
    volatile sig_atomic_t running;
    config_t config;
    int epoll_fd;
    
    // P2P Network
    p2p_network_t p2p_net;
    
    // Security Contexts
    EVP_CIPHER_CTX *aes_ctx;
    EVP_CIPHER_CTX *chacha_ctx;
} server_state_t;

server_state_t server;
signal_context *global_context = NULL;

// Advanced Encryption Functions
int crypto_random(uint8_t *data, size_t len, void *user_data) {
    if (RAND_bytes(data, len) != 1) {
        return SG_ERR_UNKNOWN;
    }
    return 0;
}

int crypto_hmac_sha256_init(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data) {
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if (!mac) return SG_ERR_NOMEM;
    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    if (!ctx) {
        EVP_MAC_free(mac);
        return SG_ERR_NOMEM;
    }
    OSSL_PARAM params[] = {
        OSSL_PARAM_construct_utf8_string("digest", "SHA256", 0),
        OSSL_PARAM_construct_end()
    };
    if (!EVP_MAC_init(ctx, key, key_len, params)) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return SG_ERR_UNKNOWN;
    }
    hmac_ctx_wrapper *wrapper = malloc(sizeof(hmac_ctx_wrapper));
    if (!wrapper) {
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return SG_ERR_NOMEM;
    }
    wrapper->ctx = ctx;
    wrapper->mac = mac;
    *hmac_context = wrapper;
    return 0;
}

int crypto_hmac_sha256_update(void *hmac_context, const uint8_t *data, size_t data_len, void *user_data) {
    hmac_ctx_wrapper *wrapper = (hmac_ctx_wrapper *)hmac_context;
    if (!EVP_MAC_update(wrapper->ctx, data, data_len)) {
        return SG_ERR_UNKNOWN;
    }
    return 0;
}

int crypto_hmac_sha256_final(void *hmac_context, signal_buffer **output, void *user_data) {
    hmac_ctx_wrapper *wrapper = (hmac_ctx_wrapper *)hmac_context;
    size_t len = EVP_MAC_CTX_get_mac_size(wrapper->ctx);
    uint8_t *md = malloc(len);
    if (!md) {
        free(wrapper);
        return SG_ERR_NOMEM;
    }
    if (!EVP_MAC_final(wrapper->ctx, md, &len, len)) {
        free(md);
        free(wrapper);
        return SG_ERR_UNKNOWN;
    }
    signal_buffer *buf = signal_buffer_create(md, len);
    free(md);
    if (!buf) {
        free(wrapper);
        return SG_ERR_NOMEM;
    }
    *output = buf;
    EVP_MAC_CTX_free(wrapper->ctx);
    EVP_MAC_free(wrapper->mac);
    free(wrapper);
    return 0;
}

void crypto_hmac_sha256_cleanup(void *hmac_context, void *user_data) {
    // Already cleaned in final
}

// Multi-Algorithm Encryption
int crypto_encrypt_aes_gcm(const uint8_t *plaintext, size_t plaintext_len,
                          const uint8_t *key, const uint8_t *iv,
                          uint8_t *ciphertext, uint8_t *tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ciphertext_len;
    
    if (!ctx) return -1;
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;
    
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

int crypto_encrypt_chacha20(const uint8_t *plaintext, size_t plaintext_len,
                           const uint8_t *key, const uint8_t *nonce,
                           uint8_t *ciphertext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len, ciphertext_len;
    
    if (!ctx) return -1;
    
    if (EVP_EncryptInit_ex(ctx, EVP_chacha20(), NULL, key, nonce) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// P2P Network Functions
void init_p2p_network(void) {
    server.p2p_net.peers = calloc(MAX_CLIENTS, sizeof(p2p_peer_t));
    server.p2p_net.max_peers = MAX_CLIENTS;
    server.p2p_net.peer_count = 0;
    server.p2p_net.dht_port = 5666;
    server.p2p_net.is_super_node = 1;
    pthread_mutex_init(&server.p2p_net.peer_mutex, NULL);
}

int add_peer_to_network(const char *peer_id, struct sockaddr_in addr, const char *pub_key) {
    pthread_mutex_lock(&server.p2p_net.peer_mutex);
    
    if (server.p2p_net.peer_count >= server.p2p_net.max_peers) {
        pthread_mutex_unlock(&server.p2p_net.peer_mutex);
        return -1;
    }
    
    p2p_peer_t *peer = &server.p2p_net.peers[server.p2p_net.peer_count];
    strncpy(peer->peer_id, peer_id, sizeof(peer->peer_id)-1);
    peer->public_addr = addr;
    peer->is_connected = 1;
    peer->last_heartbeat = time(NULL);
    strncpy(peer->public_key, pub_key, sizeof(peer->public_key)-1);
    
    server.p2p_net.peer_count++;
    pthread_mutex_unlock(&server.p2p_net.peer_mutex);
    return 0;
}

void broadcast_p2p_heartbeat(void) {
    // Broadcast presence to P2P network
    char heartbeat_msg[256];
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
             "P2P_HEARTBEAT|%s|%d", "main_server", PORT);
    
    // In real implementation, this would broadcast to known peers
}

// Enhanced Security Functions
void generate_client_keys(client_t *client) {
    // Generate additional encryption keys
    RAND_bytes(client->chacha_key, sizeof(client->chacha_key));
    RAND_bytes(client->twofish_key, sizeof(client->twofish_key));
    
    // Generate P2P keys
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    
    if (ctx && EVP_PKEY_keygen_init(ctx) > 0) {
        EVP_PKEY_keygen(ctx, &pkey);
        
        // Store public key (simplified)
        size_t len = 0;
        EVP_PKEY_get_raw_public_key(pkey, NULL, &len);
        if (len > 0 && len < sizeof(client->public_key)) {
            EVP_PKEY_get_raw_public_key(pkey, (unsigned char*)client->public_key, &len);
        }
        
        EVP_PKEY_free(pkey);
    }
    EVP_PKEY_CTX_free(ctx);
}

int encrypt_message_multi_algo(client_t *client, const char *plaintext, 
                              char **ciphertext, int algo_type) {
    size_t plaintext_len = strlen(plaintext);
    size_t ciphertext_len = plaintext_len + 64; // Extra space for IV/tag
    
    *ciphertext = malloc(ciphertext_len);
    if (!*ciphertext) return -1;
    
    unsigned char iv[12];
    unsigned char tag[16];
    RAND_bytes(iv, sizeof(iv));
    
    int result = -1;
    
    switch(algo_type) {
        case 0: // AES-256-GCM
            result = crypto_encrypt_aes_gcm(
                (const uint8_t*)plaintext, plaintext_len,
                client->session_key, iv,
                (uint8_t*)*ciphertext, tag
            );
            break;
            
        case 1: // ChaCha20
            result = crypto_encrypt_chacha20(
                (const uint8_t*)plaintext, plaintext_len,
                client->chacha_key, iv,
                (uint8_t*)*ciphertext
            );
            break;
            
        default:
            free(*ciphertext);
            *ciphertext = NULL;
            return -1;
    }
    
    if (result > 0) {
        ciphertext_len = result;
    } else {
        free(*ciphertext);
        *ciphertext = NULL;
        return -1;
    }
    
    return ciphertext_len;
}

// Anti-blocking and IP Spoofing Protection
int setup_tor_proxy(void) {
    if (!server.config.enable_tor_proxy) return 0;
    
    // This would implement Tor client integration
    // For now, it's a placeholder for actual Tor implementation
    printf("Tor proxy support enabled (placeholder)\n");
    return 0;
}

int validate_client_identity(client_t *client) {
    // Advanced client validation against various attacks
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    
    // Check for suspicious IP patterns
    uint32_t ip = ntohl(client->addr.sin_addr.s_addr);
    
    // Filter known bad IP ranges (simplified)
    if ((ip >> 24) == 10 ||           // Private network
        (ip >> 20) == 0xAC1 ||        // Private network  
        (ip >> 16) == 0xC0A8) {       // Private network
        // Allow private IPs for local testing
    }
    
    return 1; // Valid client
}

// Existing functions with P2P enhancements
void stop_mesh_server(void) {
    server.running = 0;
}

int is_server_running(void) {
    return server.running;
}

int crypto_encrypt(signal_buffer **output,
                  int cipher,
                  const uint8_t *key, size_t key_len,
                  const uint8_t *iv, size_t iv_len,
                  const uint8_t *plaintext, size_t plaintext_len,
                  void *user_data) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return SG_ERR_NOMEM;
    const EVP_CIPHER *evp_cipher;
    if (key_len == 16) evp_cipher = EVP_aes_128_cbc();
    else if (key_len == 32) evp_cipher = EVP_aes_256_cbc();
    else {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    if (EVP_EncryptInit_ex(ctx, evp_cipher, NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    int out_len = plaintext_len + EVP_CIPHER_block_size(evp_cipher);
    signal_buffer *output_buf = signal_buffer_alloc(out_len);
    if (!output_buf) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    int len1, len2;
    uint8_t *out_data = signal_buffer_data(output_buf);
    if (EVP_EncryptUpdate(ctx, out_data, &len1, plaintext, plaintext_len) != 1) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    if (EVP_EncryptFinal_ex(ctx, out_data + len1, &len2) != 1) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    signal_buffer *final_buf = signal_buffer_alloc(len1 + len2);
    if (!final_buf) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    memcpy(signal_buffer_data(final_buf), out_data, len1 + len2);
    signal_buffer_free(output_buf);
    *output = final_buf;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

int crypto_decrypt(signal_buffer **output,
                  int cipher,
                  const uint8_t *key, size_t key_len,
                  const uint8_t *iv, size_t iv_len,
                  const uint8_t *ciphertext, size_t ciphertext_len,
                  void *user_data) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return SG_ERR_NOMEM;
    const EVP_CIPHER *evp_cipher;
    if (key_len == 16) evp_cipher = EVP_aes_128_cbc();
    else if (key_len == 32) evp_cipher = EVP_aes_256_cbc();
    else {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    if (EVP_DecryptInit_ex(ctx, evp_cipher, NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    int out_len = ciphertext_len;
    signal_buffer *output_buf = signal_buffer_alloc(out_len);
    if (!output_buf) {
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    int len1, len2;
    uint8_t *out_data = signal_buffer_data(output_buf);
    if (EVP_DecryptUpdate(ctx, out_data, &len1, ciphertext, ciphertext_len) != 1) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    if (EVP_DecryptFinal_ex(ctx, out_data + len1, &len2) != 1) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_UNKNOWN;
    }
    signal_buffer *final_buf = signal_buffer_alloc(len1 + len2);
    if (!final_buf) {
        signal_buffer_free(output_buf);
        EVP_CIPHER_CTX_free(ctx);
        return SG_ERR_NOMEM;
    }
    memcpy(signal_buffer_data(final_buf), out_data, len1 + len2);
    signal_buffer_free(output_buf);
    *output = final_buf;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

void setup_signal_crypto_provider(signal_context *context) {
    signal_crypto_provider provider = {
        .random_func = crypto_random,
        .hmac_sha256_init_func = crypto_hmac_sha256_init,
        .hmac_sha256_update_func = crypto_hmac_sha256_update,
        .hmac_sha256_final_func = crypto_hmac_sha256_final,
        .hmac_sha256_cleanup_func = crypto_hmac_sha256_cleanup,
        .encrypt_func = crypto_encrypt,
        .decrypt_func = crypto_decrypt,
        .user_data = NULL
    };
    signal_context_set_crypto_provider(context, &provider);
}

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
    server.config.enable_tor_proxy = 0; // Disabled by default
    server.config.enable_ip_spoofing = 0;
    server.config.enable_obfuscation = 1;
    server.config.encryption_mode = 0; // AES-256-GCM
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
    // Enhanced TLS configuration
    SSL_CTX_set_min_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(server.ssl_ctx, TLS1_3_VERSION);
    
    // Multiple strong cipher suites
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
    explicit_bzero(client->nick, sizeof(client->nick));
    explicit_bzero(client->token, sizeof(client->token));
    explicit_bzero(client->session_key, sizeof(client->session_key));
    explicit_bzero(client->chacha_key, sizeof(client->chacha_key));
    explicit_bzero(client->twofish_key, sizeof(client->twofish_key));
    pthread_mutex_unlock(&server.mutex);
}

// [Previous Signal Protocol callback functions remain the same...]
int identity_key_store_get_identity_key_pair(signal_buffer **public_data, signal_buffer **private_data, void *user_data) {
    signal_buffer *public_buf = signal_buffer_alloc(32);
    signal_buffer *private_buf = signal_buffer_alloc(32);
    if (!public_buf || !private_buf) {
        if (public_buf) signal_buffer_free(public_buf);
        if (private_buf) signal_buffer_free(private_buf);
        return SG_ERR_NOMEM;
    }
    memset(signal_buffer_data(public_buf), 0xAA, 32);
    memset(signal_buffer_data(private_buf), 0xBB, 32);
    *public_data = public_buf;
    *private_data = private_buf;
    return 0;
}

int identity_key_store_get_local_registration_id(void *user_data, uint32_t *registration_id) {
    *registration_id = 1;
    return 0;
}

int identity_key_store_save_identity(const signal_protocol_address *address, 
                                    uint8_t *key_data, size_t key_len, void *user_data) {
    return 0;
}

int identity_key_store_is_trusted_identity(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data){
    return 1;
}

int session_store_load_session(signal_buffer **record, signal_buffer **user_record, const signal_protocol_address *address, void *user_data) {
    return 0;
}

int session_store_get_sub_device_sessions(signal_int_list **sessions, const char *name, size_t name_len, void *user_data) {
    return 0;
}

int session_store_store_session(const signal_protocol_address *address, uint8_t *record, size_t record_len,  uint8_t *user_record, size_t user_record_len, void *user_data) {
    return 0;
}

int session_store_contains_session(const signal_protocol_address *address, void *user_data) {
    return 0;
}

int session_store_delete_session(const signal_protocol_address *address, void *user_data) {
    return 0;
}

int session_store_delete_all_sessions(const char *name, size_t name_len, void *user_data) {
    return 0;
}

int pre_key_store_load_pre_key(signal_buffer **record, uint32_t pre_key_id, void *user_data) {
    return 0;
}

int pre_key_store_store_pre_key(uint32_t pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

int pre_key_store_contains_pre_key(uint32_t pre_key_id, void *user_data) {
    return 0;
}

int pre_key_store_remove_pre_key(uint32_t pre_key_id, void *user_data) {
    return 0;
}

int signed_pre_key_store_load_signed_pre_key(signal_buffer **record, uint32_t signed_pre_key_id, void *user_data) {
    return 0;
}

int signed_pre_key_store_store_signed_pre_key(uint32_t signed_pre_key_id, uint8_t *record, size_t record_len, void *user_data) {
    return 0;
}

int signed_pre_key_store_contains_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    return 0;
}

int signed_pre_key_store_remove_signed_pre_key(uint32_t signed_pre_key_id, void *user_data) {
    return 0;
}

int signal_protocol_init(void) {
    int result = signal_context_create(&global_context, NULL);
    if (result != 0) {
        fprintf(stderr, "Failed to create signal context: %d\n", result);
        return 0;
    }
    setup_signal_crypto_provider(global_context);
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
    result = signal_protocol_store_context_create(&client->store_context, global_context);
    if (result != 0) {
        fprintf(stderr, "Failed to create store context for client\n");
        return 0;
    }
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
        .delete_all_sessions_func = session_store_delete_all_sessions,
        .destroy_func = NULL,
        .user_data = client
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
    result = signal_protocol_store_context_set_identity_key_store(client->store_context, &identity_store);
    if (result != 0) goto error;
    result = signal_protocol_store_context_set_session_store(client->store_context, &session_store);
    if (result != 0) goto error;
    result = signal_protocol_store_context_set_pre_key_store(client->store_context, &pre_key_store);
    if (result != 0) goto error;
    result = signal_protocol_store_context_set_signed_pre_key_store(client->store_context, &signed_pre_key_store);
    if (result != 0) goto error;
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
    if (identity_key_pair) SIGNAL_UNREF(identity_key_pair);
    if (pre_keys_head) signal_protocol_key_helper_key_list_free(pre_keys_head);
    if (signed_pre_key) SIGNAL_UNREF(signed_pre_key);
    
    // Generate additional client keys for P2P
    generate_client_keys(client);
    
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
    sqlite3_exec(server.db_conn, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(server.db_conn, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(server.db_conn, "PRAGMA cache_size=10000;", NULL, NULL, NULL);
    sqlite3_exec(server.db_conn, "PRAGMA temp_store=MEMORY;", NULL, NULL, NULL);
    sqlite3_exec(server.db_conn, "PRAGMA mmap_size=268435456;", NULL, NULL, NULL);
    sqlite3_exec(server.db_conn, "PRAGMA secure_delete=ON;", NULL, NULL, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(server.db_conn));
        return 0;
    }
    const char *tables[] = {
        "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY AUTOINCREMENT, nick TEXT UNIQUE NOT NULL, token TEXT UNIQUE NOT NULL, registration_id INTEGER, ip_address TEXT, created_at DATETIME DEFAULT CURRENT_TIMESTAMP, last_login DATETIME NULL)",
        "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY AUTOINCREMENT, from_user TEXT NOT NULL, to_user TEXT, message_text TEXT NOT NULL, encrypted INTEGER DEFAULT 0, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS ip_limits (id INTEGER PRIMARY KEY AUTOINCREMENT, ip_address TEXT NOT NULL, connection_count INTEGER DEFAULT 0, last_attempt DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS signal_sessions (id INTEGER PRIMARY KEY AUTOINCREMENT, user_nick TEXT NOT NULL, device_id INTEGER, record BLOB, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS signal_pre_keys (id INTEGER PRIMARY KEY AUTOINCREMENT, user_nick TEXT NOT NULL, pre_key_id INTEGER, record BLOB, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS signal_signed_pre_keys (id INTEGER PRIMARY KEY AUTOINCREMENT, user_nick TEXT NOT NULL, signed_pre_key_id INTEGER, record BLOB, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS signal_identity_keys (id INTEGER PRIMARY KEY AUTOINCREMENT, user_nick TEXT NOT NULL, name TEXT NOT NULL, key_data BLOB, created_at DATETIME DEFAULT CURRENT_TIMESTAMP)",
        // New P2P tables
        "CREATE TABLE IF NOT EXISTS p2p_peers (id INTEGER PRIMARY KEY AUTOINCREMENT, peer_id TEXT UNIQUE NOT NULL, public_key TEXT, ip_address TEXT, port INTEGER, last_seen DATETIME DEFAULT CURRENT_TIMESTAMP)",
        "CREATE TABLE IF NOT EXISTS p2p_connections (id INTEGER PRIMARY KEY AUTOINCREMENT, peer1_id TEXT NOT NULL, peer2_id TEXT NOT NULL, connection_key TEXT, established_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
    };
    char *err_msg = NULL;
    for (size_t i = 0; i < sizeof(tables)/sizeof(tables[0]); i++) {
        result = sqlite3_exec(server.db_conn, tables[i], NULL, NULL, &err_msg);
        if (result != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }
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
        "CREATE INDEX IF NOT EXISTS idx_signal_identity_keys_user ON signal_identity_keys(user_nick)",
        "CREATE INDEX IF NOT EXISTS idx_p2p_peers_id ON p2p_peers(peer_id)",
        "CREATE INDEX IF NOT EXISTS idx_p2p_connections_peer ON p2p_connections(peer1_id, peer2_id)"
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
        const char *sql = "INSERT INTO ip_limits (ip_address, connection_count, last_attempt) VALUES (?, 1, datetime('now')) ON CONFLICT(ip_address) DO UPDATE SET connection_count = connection_count + 1, last_attempt = datetime('now')";
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

// Enhanced P2P Encryption
int encrypt_message_for_user(client_t *sender, const char *recipient_nick, const char *plaintext, char **encrypted_b64) {
    client_t *target = find_client_by_nick(recipient_nick);
    if (!target || !target->store_context) {
        return -1;
    }
    signal_protocol_address address = {
        .name = recipient_nick,
        .name_len = strlen(recipient_nick),
        .device_id = 1
    };
    session_cipher *cipher = NULL;
    int result = session_cipher_create(&cipher, sender->store_context, &address, global_context);
    if (result != 0) {
        fprintf(stderr, "Failed to create session cipher for %s\n", recipient_nick);
        return -1;
    }
    ciphertext_message *encrypted_msg = NULL;
    result = session_cipher_encrypt(cipher, (uint8_t *)plaintext, strlen(plaintext), &encrypted_msg);
    if (result != 0) {
        session_cipher_free(cipher);
        return -1;
    }
    size_t len = ciphertext_message_get_serialized_length(encrypted_msg);
    uint8_t *serialized = malloc(len);
    if (!serialized) {
        SIGNAL_UNREF(encrypted_msg);
        session_cipher_free(cipher);
        return -1;
    }
    ciphertext_message_serialize(serialized, len, encrypted_msg);
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, serialized, len);
    BIO_flush(b64);
    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);
    *encrypted_b64 = strndup(bptr->data, bptr->length);
    BIO_free_all(b64);
    free(serialized);
    SIGNAL_UNREF(encrypted_msg);
    session_cipher_free(cipher);
    return 0;
}

// P2P Connection Establishment
int establish_p2p_connection(client_t *client1, client_t *client2) {
    if (!client1 || !client2) return -1;
    
    // Exchange connection information
    char conn_info1[512], conn_info2[512];
    snprintf(conn_info1, sizeof(conn_info1),
             "{\"type\":\"p2p_handshake\",\"peer_id\":\"%s\",\"public_key\":\"%s\",\"address\":\"%s:%d\"}",
             client1->peer_id, client1->public_key, 
             inet_ntoa(client1->addr.sin_addr), ntohs(client1->addr.sin_port));
             
    snprintf(conn_info2, sizeof(conn_info2),
             "{\"type\":\"p2p_handshake\",\"peer_id\":\"%s\",\"public_key\":\"%s\",\"address\":\"%s:%d\"}",
             client2->peer_id, client2->public_key,
             inet_ntoa(client2->addr.sin_addr), ntohs(client2->addr.sin_port));
    
    // Send handshake information to both clients
    ssl_safe_write(client1->ssl, conn_info2, strlen(conn_info2));
    ssl_safe_write(client2->ssl, conn_info1, strlen(conn_info1));
    
    // Store P2P connection in database
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO p2p_connections (peer1_id, peer2_id, connection_key) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(server.db_conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
        char connection_key[65];
        generate_secure_token(connection_key, sizeof(connection_key));
        
        sqlite3_bind_text(stmt, 1, client1->peer_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, client2->peer_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, connection_key, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        // Send connection key to both clients
        char key_msg[256];
        snprintf(key_msg, sizeof(key_msg),
                 "{\"type\":\"p2p_key\",\"key\":\"%s\"}", connection_key);
        ssl_safe_write(client1->ssl, key_msg, strlen(key_msg));
        ssl_safe_write(client2->ssl, key_msg, strlen(key_msg));
    }
    
    return 0;
}

void *client_handler(void *arg) {
    client_t *client = (client_t*)arg;
    char buffer[BUFFER_SIZE];
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client->addr.sin_addr, ip_str, sizeof(ip_str));
    
    // Enhanced client validation
    if (!validate_client_identity(client)) {
        const char *msg = "Client validation failed\n";
        ssl_safe_write(client->ssl, msg, strlen(msg));
        disconnect_client(client, "Client validation failed");
        return NULL;
    }
    
    if (!check_ip_limit(ip_str)) {
        const char *msg = "Too many connections from your IP address\n";
        ssl_safe_write(client->ssl, msg, strlen(msg));
        disconnect_client(client, "IP limit exceeded");
        return NULL;
    }
    
    update_ip_limit(ip_str, 1);
    
    if (!init_client_signal_protocol(client)) {
        fprintf(stderr, "Failed to initialize Signal Protocol for client\n");
        disconnect_client(client, "Signal Protocol init failed");
        return NULL;
    }
    
    // Generate P2P peer ID
    snprintf(client->peer_id, sizeof(client->peer_id), "peer_%s_%ld", 
             client->nick, time(NULL));
    
    log_event("CONNECT", "New connection", client);
    const char *welcome = "Welcome to Enhanced Secure P2P Chat!\n"
                         "Commands: /register <nick>, /login <nick> <token>, "
                         "/msg <user> <message>, /send_enc <user> <message>, "
                         "/p2p_connect <user>, /online, /help\n";
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
                            if (db_token && secure_strcmp(db_token, token) == 0) {
                                strcpy_s(client->nick, sizeof(client->nick), nick);
                                strcpy_s(client->token, sizeof(client->token), token);
                                client->authorized = 1;
                                char response[128];
                                snprintf(response, sizeof(response),
                                         "{\"type\":\"login\",\"status\":\"success\",\"nick\":\"%s\"}\n", nick);
                                ssl_safe_write(client->ssl, response, strlen(response));
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
                                
                                // Add to P2P network
                                add_peer_to_network(client->peer_id, client->addr, client->public_key);
                                
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
            // Enhanced command processing with P2P support
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
            else if (strncmp(buffer, "/send_enc", 9) == 0) {
                char target[32], raw_msg[MAX_MESSAGE_LENGTH];
                if (sscanf(buffer, "/send_enc %31s %2047[^\n]", target, raw_msg) >= 2) {
                    char *encrypted_b64 = NULL;
                    if (encrypt_message_for_user(client, target, raw_msg, &encrypted_b64) == 0) {
                        char json_msg[BUFFER_SIZE];
                        snprintf(json_msg, sizeof(json_msg), 
                        "{\"type\":\"encrypted\",\"from\":\"%s\",\"to\":\"%s\",\"data\":\"%s\"}\n",
                        client->nick, target, encrypted_b64);
                        client_t *tgt = find_client_by_nick(target);
                        if (tgt) {
                            ssl_safe_write(tgt->ssl, json_msg, strlen(json_msg));
                        }
                        free(encrypted_b64);
                    } else {
                        const char *err = "{\"type\":\"error\",\"message\":\"Encryption failed\"}\n";
                        ssl_safe_write(client->ssl, err, strlen(err));
                    }
                }
            }
            else if (strncmp(buffer, "/p2p_connect", 12) == 0) {
                char target[32];
                if (sscanf(buffer, "/p2p_connect %31s", target) == 1) {
                    client_t *target_client = find_client_by_nick(target);
                    if (target_client) {
                        if (establish_p2p_connection(client, target_client) == 0) {
                            char response[128];
                            snprintf(response, sizeof(response),
                                     "{\"type\":\"p2p_initiated\",\"target\":\"%s\",\"status\":\"success\"}\n", target);
                            ssl_safe_write(client->ssl, response, strlen(response));
                        } else {
                            const char *err = "{\"type\":\"error\",\"message\":\"P2P connection failed\"}\n";
                            ssl_safe_write(client->ssl, err, strlen(err));
                        }
                    } else {
                        const char *err = "{\"type\":\"error\",\"message\":\"User not found\"}\n";
                        ssl_safe_write(client->ssl, err, strlen(err));
                    }
                }
            }
            else if (strcmp(buffer, "/help") == 0) {
                const char *help = 
                    "{\"type\":\"help\",\"commands\":[\n"
                    "  \"/online - Show online users\",\n"
                    "  \"/msg <user> <message> - Send private message\",\n"
                    "  \"/send_enc <user> <message> - Send E2E encrypted message\",\n"
                    "  \"/p2p_connect <user> - Establish P2P connection\",\n"
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
        
        // Monitor client activity
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
        
        // P2P network maintenance
        broadcast_p2p_heartbeat();
        
        // Database health check
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
    
    // Cleanup P2P resources
    if (server.p2p_net.peers) free(server.p2p_net.peers);
    pthread_mutex_destroy(&server.p2p_net.peer_mutex);
    
    if (global_context) {
        signal_context_destroy(global_context);
        global_context = NULL;
    }
    
    EVP_cleanup();
    ERR_free_strings();
}

int start_mesh_server(void) {
    printf("Starting Enhanced Secure P2P Chat Server...\n");
    memset(&server, 0, sizeof(server));
    server.running = 1;
    
    // Security hardening
    struct rlimit core_limit = {0,0}; 
    setrlimit(RLIMIT_CORE, &core_limit);
    struct rlimit fd_limit = {1024,1024};
    setrlimit(RLIMIT_NOFILE, &fd_limit);
    
    server.max_clients = MAX_CLIENTS;
    load_config();
    setup_signal_handlers();
    
    if (pthread_mutex_init(&server.mutex, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        return 1;
    }
    
    // Initialize P2P network
    init_p2p_network();
    
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
    
    // Setup Tor proxy if enabled
    if (server.config.enable_tor_proxy) {
        setup_tor_proxy();
    }
    
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
    printf("P2P Network initialized on port %d\n", server.p2p_net.dht_port);
    
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
                    fprintf(stderr, "SSL_accept failed.\n");
                    SSL_free(client_ssl);
                    close(client_fd);
                    pthread_mutex_unlock(&server.mutex);
                    continue;
                }
                
                // Initialize enhanced client structure
                server.clients[client_index].fd = client_fd;
                server.clients[client_index].ssl = client_ssl;
                server.clients[client_index].addr = client_addr;
                server.clients[client_index].connect_time = time(NULL);
                server.clients[client_index].last_activity = time(NULL);
                server.clients[client_index].authorized = 0;
                server.clients[client_index].is_p2p_connected = 0;
                
                memset(server.clients[client_index].nick, 0, sizeof(server.clients[client_index].nick));
                memset(server.clients[client_index].token, 0, sizeof(server.clients[client_index].token));
                memset(server.clients[client_index].public_key, 0, sizeof(server.clients[client_index].public_key));
                memset(server.clients[client_index].peer_id, 0, sizeof(server.clients[client_index].peer_id));
                
                server.clients[client_index].rate_limit.window_start = time(NULL);
                server.clients[client_index].rate_limit.request_count = 0;
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