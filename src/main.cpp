#include "header.h"

// Signal handler to reap background processes automatically
//OS sends to the parent shell whenever a child process terminates or stops
void sigchld_handler(int sig) {
    (void)sig; //Casts sig to silence compiler warnings regarding unused function parameters.
    int status;
    //-1: Waits for any child process
    //WNOHANG: Non-blocking flag that tells waitpid() to return immediately if no child has exited.
    // This prevents the parent from blocking indefinitely while waiting for a child to terminate.
    //waitpid() returns the PID of the terminated child, or 0 if no child has exited yet.

    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Child reaped
    }
}

// Handler for CTRL-C
void sigint_handler(int sig) {
    (void)sig;
    if (FOREGROUND_PID != -1) { //Checks if a foreground job is currently running.
        kill(FOREGROUND_PID, SIGINT); // interrupting it without terminating the main shell process.
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
        kill(FOREGROUND_PID, SIGTSTP); //Sends SIGTSTP to suspend/stop the active foreground process.
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
    // Registers custom signal handlers
    // asynchronous process cleanup (SIGCHLD),
    // keyboard interrupts (SIGINT), 
    //and process suspension (SIGTSTP).
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    // Shell Read-Eval-Print Loop (REPL)
    while (1) { //nfinite loop driving the interactive shell session.
        display_prompt(); //Renders the interactive user prompt
        char *line = read_input_line();
        reset_terminal_driver(); //Restores normal terminal modes/settings after input reading completes.

        if (line != NULL) {
            //Triggers execution: parses semicolons,
            //sets up pipes/redirections, and runs commands.
            execute_command_line(line); 
            free(line);
        }
    }

    return 0;
}