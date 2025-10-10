#include "p2p_messenger.h"
#include <ncurses.h>
#include <form.h>

typedef struct {
    p2p_network_t network;
    char username[MAX_USERNAME_LEN];
    int is_running;
    WINDOW *chat_win;
    WINDOW *input_win;
    WINDOW *users_win;
} client_state_t;

void *message_receiver(void *arg) {
    client_state_t *client = (client_state_t *)arg;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return NULL;
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        return NULL;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return NULL;
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        return NULL;
    }
    
    while (client->is_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(server_fd, &read_fds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }
            
            p2p_message_t msg;
            int bytes_received = recv(new_socket, &msg, sizeof(msg), 0);
            
            if (bytes_received > 0) {
                if (msg.type == 1) {
                    wattron(client->chat_win, COLOR_PAIR(1));
                    wprintw(client->chat_win, "\n%s: %s\n", msg.from, msg.message);
                    wattroff(client->chat_win, COLOR_PAIR(1));
                    wrefresh(client->chat_win);
                }
            }
            
            close(new_socket);
        }
    }
    
    close(server_fd);
    return NULL;
}

void run_client(client_state_t *client) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    client->chat_win = newwin(height - 4, width - 20, 0, 0);
    client->users_win = newwin(height - 4, 20, 0, width - 20);
    client->input_win = newwin(3, width, height - 3, 0);
    
    scrollok(client->chat_win, TRUE);
    scrollok(client->users_win, TRUE);
    
    pthread_t receiver_thread;
    client->is_running = 1;
    pthread_create(&receiver_thread, NULL, message_receiver, client);
    
    char input_buffer[MAX_MESSAGE_LEN];
    int ch, pos = 0;
    
    wattron(client->chat_win, COLOR_PAIR(2));
    wprintw(client->chat_win, "=== Secure P2P Messenger ===\n");
    wprintw(client->chat_win, "User: %s | Port: %d\n", client->username, PORT);
    wprintw(client->chat_win, "Key fingerprint: ");
    for (int i = 0; i < 8; i++) {
        wprintw(client->chat_win, "%02x", client->network.my_public_key[i]);
    }
    wprintw(client->chat_win, "...\n");
    wprintw(client->chat_win, "Type /help for commands\n\n");
    wattroff(client->chat_win, COLOR_PAIR(2));
    wrefresh(client->chat_win);
    
    wattron(client->users_win, COLOR_PAIR(3));
    wprintw(client->users_win, " Online Users\n");
    wprintw(client->users_win, "─────────────\n");
    wattroff(client->users_win, COLOR_PAIR(3));
    wrefresh(client->users_win);
    
    while (client->is_running) {
        wattron(client->input_win, COLOR_PAIR(4));
        mvwprintw(client->input_win, 0, 0, "Type your message (ESC to quit): ");
        wclrtoeol(client->input_win);
        wattroff(client->input_win, COLOR_PAIR(4));
        
        mvwprintw(client->input_win, 1, 0, "> ");
        wclrtoeol(client->input_win);
        wrefresh(client->input_win);
        
        pos = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        
        while ((ch = wgetch(client->input_win)) != '\n' && pos < MAX_MESSAGE_LEN - 1) {
            if (ch == KEY_BACKSPACE || ch == 127) {
                if (pos > 0) {
                    pos--;
                    input_buffer[pos] = '\0';
                    mvwprintw(client->input_win, 1, 2 + pos, " ");
                    wmove(client->input_win, 1, 2 + pos);
                }
            } else if (ch == 27) {
                client->is_running = 0;
                break;
            } else if (isprint(ch)) {
                input_buffer[pos++] = ch;
                mvwprintw(client->input_win, 1, 2 + pos - 1, "%c", ch);
            }
            wrefresh(client->input_win);
        }
        
        if (!client->is_running) break;
        
        input_buffer[pos] = '\0';
        
        if (input_buffer[0] == '/') {
            if (strncmp(input_buffer, "/quit", 5) == 0) {
                client->is_running = 0;
            } else if (strncmp(input_buffer, "/users", 6) == 0) {
                wattron(client->chat_win, COLOR_PAIR(3));
                wprintw(client->chat_win, "\n=== Online Users ===\n");
                pthread_mutex_lock(&client->network.mutex);
                for (int i = 0; i < client->network.user_count; i++) {
                    wprintw(client->chat_win, "• %s (%s:%d)\n", 
                           client->network.users[i].username,
                           client->network.users[i].ip,
                           client->network.users[i].port);
                }
                pthread_mutex_unlock(&client->network.mutex);
                wprintw(client->chat_win, "===================\n");
                wattroff(client->chat_win, COLOR_PAIR(3));
                wrefresh(client->chat_win);
            } else if (strncmp(input_buffer, "/msg", 4) == 0) {
                char target_user[MAX_USERNAME_LEN];
                char message[MAX_MESSAGE_LEN];
                
                if (sscanf(input_buffer, "/msg %s %[^\n]", target_user, message) == 2) {
                    user_info_t *target = find_user(&client->network, target_user);
                    if (target) {
                        unsigned char shared_secret[KEY_LENGTH];
                        if (derive_shared_secret(client->network.my_private_key, target->public_key, shared_secret) == 0) {
                            unsigned char ciphertext[MAX_MESSAGE_LEN];
                            unsigned char iv[16];
                            unsigned char tag[16];
                            
                            int ciphertext_len = encrypt_message_signal(
                                (unsigned char*)message, strlen(message),
                                shared_secret, ciphertext, iv, tag
                            );
                            
                            if (ciphertext_len > 0) {
                                p2p_message_t encrypted_msg;
                                memset(&encrypted_msg, 0, sizeof(encrypted_msg));
                                encrypted_msg.type = 1;
                                strcpy(encrypted_msg.from, client->username);
                                strcpy(encrypted_msg.to, target_user);
                                memcpy(encrypted_msg.iv, iv, 16);
                                memcpy(encrypted_msg.tag, tag, 16);
                                memcpy(encrypted_msg.message, ciphertext, ciphertext_len);
                                encrypted_msg.message_len = ciphertext_len;
                                
                                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                                if (sockfd >= 0) {
                                    struct sockaddr_in serv_addr;
                                    serv_addr.sin_family = AF_INET;
                                    serv_addr.sin_port = htons(target->port);
                                    inet_pton(AF_INET, target->ip, &serv_addr.sin_addr);
                                    
                                    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
                                        send(sockfd, &encrypted_msg, sizeof(encrypted_msg), 0);
                                        wattron(client->chat_win, COLOR_PAIR(1));
                                        wprintw(client->chat_win, "\n[Encrypted to %s]: %s\n", target_user, message);
                                        wattroff(client->chat_win, COLOR_PAIR(1));
                                    }
                                    close(sockfd);
                                }
                            }
                        }
                    } else {
                        wattron(client->chat_win, COLOR_PAIR(1));
                        wprintw(client->chat_win, "\nUser %s not found\n", target_user);
                        wattroff(client->chat_win, COLOR_PAIR(1));
                    }
                }
                wrefresh(client->chat_win);
            } else if (strncmp(input_buffer, "/create_group", 13) == 0) {
                char group_name[MAX_GROUP_NAME_LEN];
                if (sscanf(input_buffer, "/create_group %s", group_name) == 1) {
                    if (create_group(&client->network, group_name, client->username) == 0) {
                        wattron(client->chat_win, COLOR_PAIR(4));
                        wprintw(client->chat_win, "\nGroup '%s' created successfully!\n", group_name);
                        wattroff(client->chat_win, COLOR_PAIR(4));
                    } else {
                        wattron(client->chat_win, COLOR_PAIR(1));
                        wprintw(client->chat_win, "\nFailed to create group '%s'\n", group_name);
                        wattroff(client->chat_win, COLOR_PAIR(1));
                    }
                    wrefresh(client->chat_win);
                }
            } else if (strncmp(input_buffer, "/join_group", 11) == 0) {
                char group_name[MAX_GROUP_NAME_LEN];
                if (sscanf(input_buffer, "/join_group %s", group_name) == 1) {
                    if (join_group(&client->network, group_name, client->username) == 0) {
                        wattron(client->chat_win, COLOR_PAIR(4));
                        wprintw(client->chat_win, "\nJoined group '%s' successfully!\n", group_name);
                        wattroff(client->chat_win, COLOR_PAIR(4));
                    } else {
                        wattron(client->chat_win, COLOR_PAIR(1));
                        wprintw(client->chat_win, "\nFailed to join group '%s'\n", group_name);
                        wattroff(client->chat_win, COLOR_PAIR(1));
                    }
                    wrefresh(client->chat_win);
                }
            } else if (strncmp(input_buffer, "/groups", 7) == 0) {
                wattron(client->chat_win, COLOR_PAIR(3));
                wprintw(client->chat_win, "\n=== Your Groups ===\n");
                pthread_mutex_lock(&client->network.mutex);
                for (int i = 0; i < client->network.group_count; i++) {
                    wprintw(client->chat_win, "• %s (%d members)\n", 
                           client->network.groups[i].group_name,
                           client->network.groups[i].member_count);
                }
                pthread_mutex_unlock(&client->network.mutex);
                wprintw(client->chat_win, "==================\n");
                wattroff(client->chat_win, COLOR_PAIR(3));
                wrefresh(client->chat_win);
            } else if (strcmp(input_buffer, "/help") == 0) {
                wattron(client->chat_win, COLOR_PAIR(2));
                wprintw(client->chat_win, "\n=== Available Commands ===\n");
                wprintw(client->chat_win, "/msg <user> <message> - Send encrypted private message\n");
                wprintw(client->chat_win, "/create_group <name> - Create a new group chat\n");
                wprintw(client->chat_win, "/join_group <name> - Join existing group\n");
                wprintw(client->chat_win, "/groups - List your groups\n");
                wprintw(client->chat_win, "/users - Show online users\n");
                wprintw(client->chat_win, "/help - Show this help\n");
                wprintw(client->chat_win, "/quit - Exit messenger\n");
                wprintw(client->chat_win, "========================\n");
                wattroff(client->chat_win, COLOR_PAIR(2));
                wrefresh(client->chat_win);
            }
        } else if (strlen(input_buffer) > 0) {
            wattron(client->chat_win, COLOR_PAIR(3));
            wprintw(client->chat_win, "\n%s: %s\n", client->username, input_buffer);
            wattroff(client->chat_win, COLOR_PAIR(3));
            wrefresh(client->chat_win);
        }
    }
    
    client->is_running = 0;
    pthread_join(receiver_thread, NULL);
    
    delwin(client->chat_win);
    delwin(client->input_win);
    delwin(client->users_win);
    endwin();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <username>\n", argv[0]);
        return 1;
    }
    
    client_state_t client;
    memset(&client, 0, sizeof(client));
    strcpy(client.username, argv[1]);
    
    if (p2p_network_init(&client.network, 0) != 0) {
        printf("Failed to initialize P2P network\n");
        return 1;
    }
    
    add_user(&client.network, client.username, "127.0.0.1", PORT, client.network.my_public_key);
    
    printf("Starting Secure P2P Messenger for %s on port %d\n", client.username, PORT);
    printf("Your public key: ");
    for (int i = 0; i < KEY_LENGTH; i++) {
        printf("%02x", client.network.my_public_key[i]);
    }
    printf("\n");
    printf("Press ESC in the app to exit\n");
    sleep(2);
    
    run_client(&client);
    
    p2p_network_cleanup(&client.network);
    return 0;
}