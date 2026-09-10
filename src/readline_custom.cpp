#include "header.h"

struct termios default_termios;

// Restore standard terminal settings when exiting or executing commands
void reset_terminal_driver() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &default_termios);
}

// Enable non-canonical mode to read raw keystrokes immediately
void init_terminal_driver() {
    tcgetattr(STDIN_FILENO, &default_termios);
    atexit(reset_terminal_driver);

    struct termios raw = default_termios;
    raw.c_lflag &= ~(ECHO | ICANON); //disables ICANON and ECHO flags for raw input
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Handle TAB auto-completion for files and directory contents
// buf is the line being typed so far
// *len is how many characters are in it
// *pos is where the cursor currently sits
void autocomplete_word(char *buf, int *len, int *pos) {
    // null-terminates the buffer so C string functions can be used on it safely
    buf[*len] = '\0'; //

    // Find starting index of current target word
    int word_start = *pos - 1;
    while (word_start >= 0 && buf[word_start] != ' ' && buf[word_start] != '\t') {
        word_start--;
    }
    word_start++;

    char *prefix = buf + word_start; //the current word being typed, starting from the first character of the word
    size_t prefix_len = *pos - word_start;

    char *matches[256];
    int match_count = 0;

    DIR *dir = opendir("."); //open current working directory 
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, prefix, prefix_len) == 0) { //check every entry name against prefix only upto prefix len
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    matches[match_count] = strdup(entry->d_name); //if match, duplicate the entry name and store in matches array
                    match_count++;
                }
            }
        }
        closedir(dir);
    }

    // Exact single match: auto-complete and append trailing space
    if (match_count == 1) {
        const char *match = matches[0];
        size_t append_len = strlen(match) - prefix_len;

        memmove(buf + *pos + append_len, buf + *pos, *len - *pos + 1);
        memcpy(buf + *pos, match + prefix_len, append_len);

        *len += append_len;
        *pos += append_len;

        buf[*len] = '\0';
        printf("%s ", buf + *pos - append_len);
        buf[*len] = ' ';
        (*len)++;
        (*pos)++;
        buf[*len] = '\0';
        fflush(stdout);
    } 
    // Multiple matches: display choices and redraw line prompt
    else if (match_count > 1) {
        printf("\n");
        for (int i = 0; i < match_count; i++) {
            printf("%s  ", matches[i]);
        }
        printf("\n");

        display_prompt();
        printf("%s", buf);

        for (int i = *len; i > *pos; i--) {
            printf("\b");
        }
        fflush(stdout);
    }

    for (int i = 0; i < match_count; i++) {
        free(matches[i]);
    }
}

// Redraw characters on line following cursor after edits or backspace
static void redraw_line_suffix(const char *buf, int len, int pos) {
    printf("%s", buf + pos);
    printf(" "); // Clear trailing character leftover from backspace
    for (int i = len + 1; i > pos; i--) {
        printf("\b");
    }
    fflush(stdout);
}

// Custom interactive command line reader supporting TAB, backspace, and arrow keys
char* read_input_line() {
    init_terminal_driver();

    char *buf = (char*)malloc(MAX_CMD_LEN);
    int len = 0;
    int pos = 0;
    buf[0] = '\0';

    int history_index = get_history_count();
    char ch;

    while (read(STDIN_FILENO, &ch, 1) == 1) {
        // CTRL-D handling
        if (ch == 4) { 
            if (len == 0) {
                reset_terminal_driver();
                printf("\n");
                exit(0);
            }
        } 
        // Enter Key
        else if (ch == '\n') {
            printf("\n");
            buf[len] = '\0';
            break;
        } 
        // Backspace
        else if (ch == 127 || ch == 8) { 
            if (pos > 0) {
                memmove(buf + pos - 1, buf + pos, len - pos + 1);
                pos--;
                len--;
                printf("\b");
                redraw_line_suffix(buf, len, pos);
            }
        } 
        // TAB Autocomplete
        else if (ch == '\t') { 
            autocomplete_word(buf, &len, &pos);
        } 
        // ANSI Escape Sequences (Arrow Navigation)
        else if (ch == 27) { 
            char sequence[2];
            if (read(STDIN_FILENO, &sequence[0], 1) == 1 && read(STDIN_FILENO, &sequence[1], 1) == 1) {
                if (sequence[0] == '[') {
                    // UP Arrow
                    if (sequence[1] == 'A') { 
                        if (history_index > 0) {
                            history_index--;
                            char *hist_cmd = get_history_at(history_index);

                            if (hist_cmd != NULL) {
                                while (pos > 0) { printf("\b"); pos--; }
                                for (int i = 0; i < len; i++) printf(" ");
                                for (int i = 0; i < len; i++) printf("\b");

                                strcpy(buf, hist_cmd);
                                len = strlen(buf);
                                pos = len;
                                printf("%s", buf);
                                fflush(stdout);
                            }
                        }
                    } 
                    // DOWN Arrow
                    else if (sequence[1] == 'B') { 
                        if (history_index < get_history_count() - 1) {
                            history_index++;
                            char *hist_cmd = get_history_at(history_index);

                            if (hist_cmd != NULL) {
                                while (pos > 0) { printf("\b"); pos--; }
                                for (int i = 0; i < len; i++) printf(" ");
                                for (int i = 0; i < len; i++) printf("\b");

                                strcpy(buf, hist_cmd);
                                len = strlen(buf);
                                pos = len;
                                printf("%s", buf);
                                fflush(stdout);
                            }
                        } 
                        else if (history_index == get_history_count() - 1) {
                            history_index++;

                            while (pos > 0) { printf("\b"); pos--; }
                            for (int i = 0; i < len; i++) printf(" ");
                            for (int i = 0; i < len; i++) printf("\b");

                            buf[0] = '\0';
                            len = 0;
                            pos = 0;
                            fflush(stdout);
                        }
                    } 
                    // RIGHT Arrow
                    else if (sequence[1] == 'C') { 
                        if (pos < len) {
                            printf("\033[C");
                            pos++;
                            fflush(stdout);
                        }
                    } 
                    // LEFT Arrow
                    else if (sequence[1] == 'D') { 
                        if (pos > 0) {
                            printf("\b");
                            pos--;
                            fflush(stdout);
                        }
                    }
                }
            }
        } 
        // Normal Key Insertion
        else {
            if (len < MAX_CMD_LEN - 1) {
                memmove(buf + pos + 1, buf + pos, len - pos + 1);
                buf[pos] = ch;
                len++;
                printf("%c", ch);
                pos++;
                redraw_line_suffix(buf, len, pos);
            }
        }
    }

    reset_terminal_driver();
    return buf;
}