#include "p2p_messenger.h"

// === Signal Protocol Storage ===
static int get_identity_key_pair(signal_buffer **public_data, signal_buffer **private_data, void *user_data) {
    p2p_network_t *net = (p2p_network_t*)user_data;
    ec_key_pair *key_pair;
    curve_generate_key_pair(net->global_context, &key_pair);
    *public_data = ec_public_key_serialize(ec_key_pair_get_public(key_pair));
    *private_data = ec_private_key_serialize(ec_key_pair_get_private(key_pair));
    SIGNAL_UNREF(key_pair);
    return 0;
}

static int get_local_registration_id(void *user_data, uint32_t *registration_id) {
    *registration_id = 1;
    return 0;
}

static int store_identity_key(const signal_protocol_address *address, uint8_t *key_data, size_t key_len, void *user_data) {
    // Save to DB
    return 0;
}

// ... (остальные callback'и для Signal Protocol)

int p2p_network_init(p2p_network_t *net, const char *username) {
    memset(net, 0, sizeof(*net));
    strncpy(net->local_username, username, MAX_USERNAME_LEN - 1);
    pthread_mutex_init(&net->mutex, NULL);

    // Инициализация SQLite
    if (sqlite3_open(DB_PATH, &net->db) != SQLITE_OK) return -1;

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, ip TEXT, port INTEGER);"
        "CREATE TABLE IF NOT EXISTS messages ("
        "   id INTEGER PRIMARY KEY, from_user TEXT, to_user TEXT, group_name TEXT,"
        "   ciphertext BLOB, iv BLOB, tag BLOB, is_group INTEGER, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS groups (name TEXT PRIMARY KEY, key BLOB);"
        "CREATE TABLE IF NOT EXISTS group_members (group_name TEXT, username TEXT);";
    sqlite3_exec(net->db, sql, NULL, NULL, NULL);

    // Инициализация Signal Protocol
    signal_context_create(&net->global_context, NULL);
    signal_protocol_store_context_create(&net->store, net->global_context);
    
    signal_protocol_store_context_set_identity_key_store(net->store, ...); // реализуйте
    signal_protocol_store_context_set_pre_key_store(net->store, ...);
    signal_protocol_store_context_set_signed_pre_key_store(net->store, ...);
    signal_protocol_store_context_set_session_store(net->store, ...);

    key_helper_pre_key_list *pre_keys;
    key_helper_generate_pre_keys(&pre_keys, 1, 100, net->global_context);
    // Сохраните pre-keys в store

    return 0;
}

// === Шифрование для пользователя ===
int encrypt_for_user(p2p_network_t *net, const char *to, const char *plaintext, unsigned char *ciphertext, size_t *ciphertext_len) {
    user_info_t *user = find_user(net, to);
    if (!user) return -1;

    session_cipher *cipher;
    session_cipher_create(&cipher, net->store, &user->addr, net->global_context);
    
    ciphertext_message *encrypted_msg;
    session_cipher_encrypt(cipher, (uint8_t*)plaintext, strlen(plaintext), &encrypted_msg);
    
    signal_buffer *buffer = ciphertext_message_get_serialized(encrypted_msg);
    *ciphertext_len = signal_buffer_len(buffer);
    memcpy(ciphertext, signal_buffer_data(buffer), *ciphertext_len);

    SIGNAL_UNREF(encrypted_msg);
    SIGNAL_UNREF(cipher);
    return 0;
}

// === Приём сообщений ===
void *message_receiver(void *arg) {
    client_state_t *client = arg;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // ... bind, listen ...

    while (client->is_running) {
        // accept + recv
        p2p_message_t msg;
        recv(..., &msg, sizeof(msg), 0);

        if (msg.type == 1) {
            char plaintext[MAX_MESSAGE_LEN];
            if (decrypt_from_user(&client->network, msg.from, msg.ciphertext, msg.ciphertext_len, plaintext) == 0) {
                // Показать в chat_win
                wprintw(client->chat_win, "\n%s: %s\n", msg.from, plaintext);
                save_message(&client->network, msg.from, client->username, NULL, msg.ciphertext, msg.ciphertext_len, 0);
            }
        }
    }
    return NULL;
}