#ifndef MESSENGER_H
#define MESSENGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sqlite3.h>
#include <signal.h>
#include <signal/signal_protocol.h>
#include <signal/session_builder.h>
#include <signal/session_cipher.h>
#include <signal/key_helper.h>
#include <ncurses.h>

#define MAX_USERS 100
#define MAX_MESSAGE_LEN 4096
#define MAX_USERNAME_LEN 32
#define MAX_GROUP_NAME_LEN 32
#define DEFAULT_PORT 8888
#define DB_FILE "messenger.db"

typedef struct {
    char username[MAX_USERNAME_LEN];
    char ip[INET6_ADDRSTRLEN];
    int port;
    signal_protocol_address addr;
    time_t last_seen;
} user_info_t;

typedef struct {
    char group_name[MAX_GROUP_NAME_LEN];
    char members[MAX_USERS][MAX_USERNAME_LEN];
    int member_count;
    unsigned char group_key[32];
} group_chat_t;

typedef struct {
    user_info_t users[MAX_USERS];
    group_chat_t groups[MAX_USERS];
    int user_count;
    int group_count;
    pthread_mutex_t mutex;
    sqlite3 *db;
    signal_protocol_store_context *store;
    signal_protocol_global_context *global_context;
    char local_username[MAX_USERNAME_LEN];
    int local_port;
} p2p_network_t;

typedef struct {
    p2p_network_t net;
    char username[MAX_USERNAME_LEN];
    int is_running;
    WINDOW *chat_win, *input_win, *users_win;
    int port;
} client_state_t;

int init_network(p2p_network_t *net, const char *username, int port);
void cleanup_network(p2p_network_t *net);
int add_user(p2p_network_t *net, const char *username, const char *ip, int port);
user_info_t *find_user(p2p_network_t *net, const char *username);
int create_group(p2p_network_t *net, const char *name);
int join_group(p2p_network_t *net, const char *name, const char *user);
int send_direct_message(p2p_network_t *net, const char *to, const char *msg);
int send_group_message(p2p_network_t *net, const char *group, const char *msg);
void *receiver_thread(void *arg);
int save_message(p2p_network_t *net, const char *from, const char *to, const char *group, const void *data, size_t len, int is_group);
int load_history(p2p_network_t *net, const char *peer, WINDOW *win);
void run_client(client_state_t *state);

#endif