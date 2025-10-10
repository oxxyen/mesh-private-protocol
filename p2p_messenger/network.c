#include "p2p_messenger.h"
#include "store.c"

static int db_exec(p2p_network_t *net, const char *sql) {
    char *err = 0;
    int rc = sqlite3_exec(net->db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    return rc;
}

int init_network(p2p_network_t *net, const char *username, int port) {
    memset(net, 0, sizeof(*net));
    strncpy(net->local_username, username, MAX_USERNAME_LEN - 1);
    net->local_port = port;
    pthread_mutex_init(&net->mutex, NULL);

    if (sqlite3_open(DB_FILE, &net->db) != SQLITE_OK) return -1;

    db_exec(net, "CREATE TABLE IF NOT EXISTS users(username TEXT PRIMARY KEY, ip TEXT, port INTEGER);");
    db_exec(net, "CREATE TABLE IF NOT EXISTS messages(id INTEGER PRIMARY KEY, from_user TEXT, to_user TEXT, group_name TEXT, data BLOB, is_group INTEGER, ts DATETIME DEFAULT CURRENT_TIMESTAMP);");
    db_exec(net, "CREATE TABLE IF NOT EXISTS groups(name TEXT PRIMARY KEY, key BLOB);");
    db_exec(net, "CREATE TABLE IF NOT EXISTS group_members(group_name TEXT, username TEXT);");

    signal_context_create(&net->global_context, 0);
    init_signal_store(net);

    ec_key_pair *identity;
    curve_generate_key_pair(net->global_context, &identity);
    SIGNAL_UNREF(identity);

    return 0;
}

void cleanup_network(p2p_network_t *net) {
    if (net->db) sqlite3_close(net->db);
    if (net->store) SIGNAL_UNREF(net->store);
    if (net->global_context) SIGNAL_UNREF(net->global_context);
    pthread_mutex_destroy(&net->mutex);
}

int add_user(p2p_network_t *net, const char *username, const char *ip, int port) {
    pthread_mutex_lock(&net->mutex);
    for (int i = 0; i < net->user_count; i++) {
        if (strcmp(net->users[i].username, username) == 0) {
            strcpy(net->users[i].ip, ip);
            net->users[i].port = port;
            net->users[i].last_seen = time(0);
            pthread_mutex_unlock(&net->mutex);
            return 0;
        }
    }
    if (net->user_count < MAX_USERS) {
        user_info_t *u = &net->users[net->user_count];
        strcpy(u->username, username);
        strcpy(u->ip, ip);
        u->port = port;
        u->last_seen = time(0);
        signal_protocol_address_init(&u->addr, username, strlen(username), 1);
        net->user_count++;
        pthread_mutex_unlock(&net->mutex);
        return 0;
    }
    pthread_mutex_unlock(&net->mutex);
    return -1;
}

user_info_t *find_user(p2p_network_t *net, const char *username) {
    for (int i = 0; i < net->user_count; i++) {
        if (strcmp(net->users[i].username, username) == 0) {
            return &net->users[i];
        }
    }
    return 0;
}

int create_group(p2p_network_t *net, const char *name) {
    pthread_mutex_lock(&net->mutex);
    for (int i = 0; i < net->group_count; i++) {
        if (strcmp(net->groups[i].group_name, name) == 0) {
            pthread_mutex_unlock(&net->mutex);
            return -1;
        }
    }
    if (net->group_count < MAX_USERS) {
        group_chat_t *g = &net->groups[net->group_count];
        strcpy(g->group_name, name);
        strcpy(g->members[0], net->local_username);
        g->member_count = 1;
        RAND_bytes(g->group_key, 32);
        net->group_count++;
        pthread_mutex_unlock(&net->mutex);
        return 0;
    }
    pthread_mutex_unlock(&net->mutex);
    return -1;
}

int join_group(p2p_network_t *net, const char *name, const char *user) {
    pthread_mutex_lock(&net->mutex);
    for (int i = 0; i < net->group_count; i++) {
        if (strcmp(net->groups[i].group_name, name) == 0) {
            for (int j = 0; j < net->groups[i].member_count; j++) {
                if (strcmp(net->groups[i].members[j], user) == 0) {
                    pthread_mutex_unlock(&net->mutex);
                    return -1;
                }
            }
            if (net->groups[i].member_count < MAX_USERS) {
                strcpy(net->groups[i].members[net->groups[i].member_count], user);
                net->groups[i].member_count++;
                pthread_mutex_unlock(&net->mutex);
                return 0;
            }
        }
    }
    pthread_mutex_unlock(&net->mutex);
    return -1;
}

static int encrypt_aes_gcm(const unsigned char *plaintext, size_t len, const unsigned char *key,
                          unsigned char *ciphertext, unsigned char *iv, unsigned char *tag) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int outlen, ciphertext_len = 0;
    if (!ctx) return -1;
    if (!RAND_bytes(iv, 12)) goto err;
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) goto err;
    if (EVP_EncryptUpdate(ctx, ciphertext, &outlen, plaintext, len) != 1) goto err;
    ciphertext_len = outlen;
    if (EVP_EncryptFinal_ex(ctx, ciphertext + outlen, &outlen) != 1) goto err;
    ciphertext_len += outlen;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) goto err;
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
err:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
}

static int decrypt_aes_gcm(const unsigned char *ciphertext, size_t len, const unsigned char *key,
                          const unsigned char *iv, const unsigned char *tag, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int outlen, plaintext_len = 0;
    if (!ctx) return -1;
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv) != 1) goto err;
    if (EVP_DecryptUpdate(ctx, plaintext, &outlen, ciphertext, len) != 1) goto err;
    plaintext_len = outlen;
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag) != 1) goto err;
    if (EVP_DecryptFinal_ex(ctx, plaintext + outlen, &outlen) <= 0) goto err;
    plaintext_len += outlen;
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
err:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
}

int send_direct_message(p2p_network_t *net, const char *to, const char *msg) {
    user_info_t *u = find_user(net, to);
    if (!u) return -1;

    session_builder *builder;
    session_builder_create(&builder, net->store, &u->addr, net->global_context);
    session_builder_process_pre_key_bundle(builder, 0);
    SIGNAL_UNREF(builder);

    session_cipher *cipher;
    session_cipher_create(&cipher, net->store, &u->addr, net->global_context);
    ciphertext_message *encrypted;
    session_cipher_encrypt(cipher, (uint8_t*)msg, strlen(msg), &encrypted);
    signal_buffer *buf = ciphertext_message_get_serialized(encrypted);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { SIGNAL_UNREF(cipher); SIGNAL_UNREF(encrypted); return -1; }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(u->port);
    inet_pton(AF_INET, u->ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        SIGNAL_UNREF(cipher); SIGNAL_UNREF(encrypted);
        return -1;
    }

    size_t len = signal_buffer_len(buf);
    send(sock, &len, sizeof(len), 0);
    send(sock, signal_buffer_data(buf), len, 0);
    close(sock);

    save_message(net, net->local_username, to, 0, signal_buffer_data(buf), len, 0);

    SIGNAL_UNREF(encrypted);
    SIGNAL_UNREF(cipher);
    return 0;
}

int send_group_message(p2p_network_t *net, const char *group_name, const char *msg) {
    for (int i = 0; i < net->group_count; i++) {
        if (strcmp(net->groups[i].group_name, group_name) == 0) {
            unsigned char ciphertext[MAX_MESSAGE_LEN];
            unsigned char iv[12], tag[16];
            int len = encrypt_aes_gcm((unsigned char*)msg, strlen(msg), net->groups[i].group_key, ciphertext, iv, tag);
            if (len <= 0) return -1;

            for (int j = 0; j < net->groups[i].member_count; j++) {
                if (strcmp(net->groups[i].members[j], net->local_username) == 0) continue;
                user_info_t *u = find_user(net, net->groups[i].members[j]);
                if (!u) continue;

                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) continue;
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(u->port);
                inet_pton(AF_INET, u->ip, &addr.sin_addr);
                if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    close(sock);
                    continue;
                }

                // Отправить: group_name, iv, tag, ciphertext
                send(sock, group_name, MAX_GROUP_NAME_LEN, 0);
                send(sock, iv, 12, 0);
                send(sock, tag, 16, 0);
                uint32_t clen = len;
                send(sock, &clen, 4, 0);
                send(sock, ciphertext, len, 0);
                close(sock);
            }

            save_message(net, net->local_username, 0, group_name, msg, strlen(msg), 1);
            return 0;
        }
    }
    return -1;
}

int save_message(p2p_network_t *net, const char *from, const char *to, const char *group, const void *data, size_t len, int is_group) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO messages(from_user, to_user, group_name, data, is_group) VALUES(?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(net->db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, from, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, to ? to : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, group ? group : "", -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 4, data, len, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, is_group);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int load_history(p2p_network_t *net, const char *peer, WINDOW *win) {
    sqlite3_stmt *stmt;
    char sql[512];
    if (peer && strcmp(peer, "ALL") != 0) {
        snprintf(sql, sizeof(sql), "SELECT from_user, to_user, group_name, data, is_group, ts FROM messages WHERE (from_user='%s' AND to_user='%s') OR (from_user='%s' AND to_user='%s') OR group_name='%s' ORDER BY ts", peer, net->local_username, net->local_username, peer, peer);
    } else {
        strcpy(sql, "SELECT from_user, to_user, group_name, data, is_group, ts FROM messages ORDER BY ts");
    }
    if (sqlite3_prepare_v2(net->db, sql, -1, &stmt, 0) != SQLITE_OK) return -1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *from = (const char*)sqlite3_column_text(stmt, 0);
        const char *to = (const char*)sqlite3_column_text(stmt, 1);
        const char *group = (const char*)sqlite3_column_text(stmt, 2);
        const void *data = sqlite3_column_blob(stmt, 3);
        int is_group = sqlite3_column_int(stmt, 4);
        if (is_group) {
            wprintw(win, "[Group %s] %s: (decrypted)\n", group, from);
        } else {
            wprintw(win, "%s: (encrypted)\n", from);
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

void *receiver_thread(void *arg) {
    client_state_t *state = arg;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(state->port);
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    while (state->is_running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (client_fd < 0) continue;

        size_t msg_len;
        recv(client_fd, &msg_len, sizeof(msg_len), 0);
        if (msg_len > MAX_MESSAGE_LEN) { close(client_fd); continue; }

        unsigned char *buf = malloc(msg_len);
        recv(client_fd, buf, msg_len, 0);

        // Попытка расшифровать как direct
        session_cipher *cipher;
        signal_protocol_address remote_addr = {"REMOTE", 6, 1};
        session_cipher_create(&cipher, state->net.store, &remote_addr, state->net.global_context);
        plaintext_message *plaintext;
        int result = session_cipher_decrypt(cipher, buf, msg_len, &plaintext);
        if (result == 0) {
            char *text = (char*)plaintext_message_get_plaintext(plaintext);
            wprintw(state->chat_win, "\n[Decrypted] REMOTE: %s\n", text);
            wrefresh(state->chat_win);
            SIGNAL_UNREF(plaintext);
        }
        SIGNAL_UNREF(cipher);
        free(buf);
        close(client_fd);
    }
    close(server_fd);
    return 0;
}