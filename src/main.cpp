#include "header.h"

// Signal handler to reap background processes automatically
void sigchld_handler(int sig) {
    (void)sig;
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Child reaped
    }
}

// Handler for CTRL-C
void sigint_handler(int sig) {
    (void)sig;
    if (FOREGROUND_PID != -1) {
        kill(FOREGROUND_PID, SIGINT);
    } else {
        printf("\n");
        display_prompt();
        fflush(stdout);
    }
}

// Handler for CTRL-Z
void sigtstp_handler(int sig) {
    (void)sig;
    if (FOREGROUND_PID != -1) {
        kill(FOREGROUND_PID, SIGTSTP);
    } else {
        printf("\n");
        display_prompt();
        fflush(stdout);
    }
}

int main() {
    // Save shell invocation directory as home
    if (getcwd(HOME_DIR, sizeof(HOME_DIR)) == NULL) {
        perror("getcwd");
        return 1;
    }

    init_history();

    // Set up signal handling
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    // Shell REPL
    while (1) {
        display_prompt();
        char *line = read_input_line();
        reset_terminal_driver();

        if (line != NULL) {
            execute_command_line(line);
            free(line);
        }
    }

    return 0;
}