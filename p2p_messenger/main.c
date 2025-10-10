#include "p2p_messenger.h"

void run_client(client_state_t *state) {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);

    int h, w; getmaxyx(stdscr, h, w);
    state->chat_win = newwin(h-4, w-20, 0, 0);
    state->users_win = newwin(h-4, 20, 0, w-20);
    state->input_win = newwin(3, w, h-3, 0);
    scrollok(state->chat_win, TRUE);
    scrollok(state->users_win, TRUE);

    wattron(state->chat_win, COLOR_PAIR(2));
    wprintw(state->chat_win, "=== Secure P2P Messenger (Signal Protocol) ===\n");
    wprintw(state->chat_win, "User: %s | Port: %d\n", state->username, state->port);
    wattroff(state->chat_win, COLOR_PAIR(2));
    wrefresh(state->chat_win);

    load_history(&state->net, "ALL", state->chat_win);

    pthread_t recv_thr;
    state->is_running = 1;
    pthread_create(&recv_thr, NULL, receiver_thread, state);

    char input[MAX_MESSAGE_LEN];
    while (state->is_running) {
        wattron(state->input_win, COLOR_PAIR(4));
        mvwprintw(state->input_win, 0, 0, "ESC=quit | /help");
        wattroff(state->input_win, COLOR_PAIR(4));
        mvwprintw(state->input_win, 1, 0, "> ");
        wclrtoeol(state->input_win);
        wrefresh(state->input_win);

        int pos = 0;
        int ch;
        while ((ch = wgetch(state->input_win)) != '\n') {
            if (ch == 27) { state->is_running = 0; break; }
            if (ch == KEY_BACKSPACE || ch == 127) {
                if (pos > 0) { pos--; input[pos] = 0; mvwprintw(state->input_win, 1, 2+pos, " "); wmove(state->input_win, 1, 2+pos); }
            } else if (isprint(ch) && pos < MAX_MESSAGE_LEN-1) {
                input[pos++] = ch; mvwprintw(state->input_win, 1, 2+pos-1, "%c", ch);
            }
            wrefresh(state->input_win);
        }
        if (!state->is_running) break;
        input[pos] = 0;

        if (input[0] == '/') {
            if (strncmp(input, "/quit", 5) == 0) { state->is_running = 0; }
            else if (strncmp(input, "/help", 5) == 0) {
                wprintw(state->chat_win, "\n/help /quit /msg <user> <text> /create_group <name> /join_group <name> /users\n");
            }
            else if (strncmp(input, "/msg ", 5) == 0) {
                char to[32], msg[MAX_MESSAGE_LEN];
                if (sscanf(input, "/msg %31s %[^\n]", to, msg) == 2) {
                    if (send_direct_message(&state->net, to, msg) == 0) {
                        wprintw(state->chat_win, "\n[Sent to %s]\n", to);
                    } else {
                        wprintw(state->chat_win, "\n[Failed to send to %s]\n", to);
                    }
                }
            }
            else if (strncmp(input, "/create_group ", 14) == 0) {
                char name[32];
                if (sscanf(input, "/create_group %31s", name) == 1) {
                    if (create_group(&state->net, name) == 0) {
                        wprintw(state->chat_win, "\n[Group %s created]\n", name);
                    }
                }
            }
            else if (strncmp(input, "/join_group ", 12) == 0) {
                char name[32], user[32];
                if (sscanf(input, "/join_group %31s", name) == 1) {
                    if (join_group(&state->net, name, state->username) == 0) {
                        wprintw(state->chat_win, "\n[Joined group %s]\n", name);
                    }
                }
            }
            else if (strncmp(input, "/users", 6) == 0) {
                wprintw(state->chat_win, "\nUsers:\n");
                for (int i = 0; i < state->net.user_count; i++) {
                    wprintw(state->chat_win, "- %s (%s:%d)\n", state->net.users[i].username, state->net.users[i].ip, state->net.users[i].port);
                }
            }
        } else if (strlen(input) > 0) {
            wprintw(state->chat_win, "\n%s: %s\n", state->username, input);
        }
        wrefresh(state->chat_win);
    }

    state->is_running = 0;
    pthread_join(recv_thr, NULL);
    delwin(state->chat_win); delwin(state->input_win); delwin(state->users_win);
    endwin();
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <username> <port>\n", argv[0]);
        return 1;
    }

    client_state_t state = {0};
    strncpy(state.username, argv[1], MAX_USERNAME_LEN - 1);
    state.port = atoi(argv[2]);

    if (init_network(&state.net, state.username, state.port) != 0) {
        fprintf(stderr, "Failed to init network\n");
        return 1;
    }

    add_user(&state.net, state.username, "127.0.0.1", state.port);
    printf("Starting messenger for %s on port %d\n", state.username, state.port);
    sleep(1);
    run_client(&state);
    cleanup_network(&state.net);
    return 0;
}