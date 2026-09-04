#include "header.h"

// Static ring buffer to store up to MAX_HIST_SIZE commands in memory
static char history_list[MAX_HIST_SIZE][MAX_CMD_LEN];
static int history_count = 0;

// Load history from ~/.shell_history on shell startup
void init_history() {
    char hist_path[2048];
    sprintf(hist_path, "%s/.shell_history", HOME_DIR);

    FILE *file = fopen(hist_path, "r");
    if (file == NULL) {
        return;
    }

    char line[MAX_CMD_LEN];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) > 0) {
            // Shift array left if history buffer is full
            if (history_count >= MAX_HIST_SIZE) {
                for (int i = 0; i < MAX_HIST_SIZE - 1; i++) {
                    strcpy(history_list[i], history_list[i + 1]);
                }
                strcpy(history_list[MAX_HIST_SIZE - 1], trimmed);
            } else {
                strcpy(history_list[history_count], trimmed);
                history_count++;
            }
        }
    }

    fclose(file);
}

// Persist current history buffer to disk
void save_history() {
    char hist_path[2048];
    sprintf(hist_path, "%s/.shell_history", HOME_DIR);

    FILE *file = fopen(hist_path, "w");
    if (file == NULL) {
        return;
    }

    for (int i = 0; i < history_count; i++) {
        fprintf(file, "%s\n", history_list[i]);
    }

    fclose(file);
}

// Add new command to history buffer and sync with file
void add_history(const char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) {
        return;
    }

    // Do not add consecutive duplicate commands
    if (history_count > 0 && strcmp(history_list[history_count - 1], cmd) == 0) {
        return;
    }

    // Maintain max command limit by shifting elements
    if (history_count >= MAX_HIST_SIZE) {
        for (int i = 0; i < MAX_HIST_SIZE - 1; i++) {
            strcpy(history_list[i], history_list[i + 1]);
        }
        strcpy(history_list[MAX_HIST_SIZE - 1], cmd);
    } else {
        strcpy(history_list[history_count], cmd);
        history_count++;
    }

    save_history();
}

// Display recent history commands
void display_history(int count) {
    int start_index = history_count - count;
    if (start_index < 0) {
        start_index = 0;
    }

    for (int i = start_index; i < history_count; i++) {
        printf("%s\n", history_list[i]);
    }
}

// Return current number of history entries stored
int get_history_count() {
    return history_count;
}

// Retrieve command string at a specific index for UP/DOWN arrow keys
char* get_history_at(int index) {
    if (index >= 0 && index < history_count) {
        return history_list[index];
    }
    return NULL;
}