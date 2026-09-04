#include "header.h"

pid_t FOREGROUND_PID = -1;

// Helper to execute a single command within a pipeline stage
void execute_single_command_in_pipeline(char *cmd_str, int in_fd, int out_fd, pid_t *child_pid) {
    int token_count = 0;
    char **tokens = split_string(cmd_str, " \t", &token_count);

    if (tokens == NULL || token_count == 0) {
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    char *infile = NULL;
    char *outfile = NULL;
    bool append_mode = false;

    char **clean_args = (char**)malloc((token_count + 1) * sizeof(char*));
    int clean_count = 0;

    // Parse input/output redirection symbols and build clean argument array
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (tokens[i + 1] != NULL) {
                infile = tokens[i + 1];
                i++;
            }
        } 
        else if (strcmp(tokens[i], ">") == 0) {
            if (tokens[i + 1] != NULL) {
                outfile = tokens[i + 1];
                append_mode = false;
                i++;
            }
        } 
        else if (strcmp(tokens[i], ">>") == 0) {
            if (tokens[i + 1] != NULL) {
                outfile = tokens[i + 1];
                append_mode = true;
                i++;
            }
        } 
        else {
            clean_args[clean_count] = tokens[i];
            clean_count++;
        }
    }
    clean_args[clean_count] = NULL;

    if (clean_args[0] == NULL) {
        free(clean_args);
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    // Handle internal built-in execution in parent process
    if (is_builtin(clean_args[0])) {
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);

        // Input redirection setup
        if (infile != NULL) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { 
                perror("Input redirection"); 
                free(clean_args); 
                free_token_array(tokens); 
                *child_pid = -1;
                return; 
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        } 
        else if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
        }

        // Output redirection setup
        if (outfile != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (append_mode) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            int fd = open(outfile, flags, 0644);
            if (fd < 0) { 
                perror("Output redirection"); 
                free(clean_args); 
                free_token_array(tokens); 
                *child_pid = -1;
                return; 
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        } 
        else if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
        }

        execute_builtin(clean_args);

        // Restore standard I/O descriptors
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);

        free(clean_args);
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    // System binary execution using fork and execvp
    pid_t pid = fork();

    if (pid == 0) {
        // Create new process group
        setpgid(0, 0);

        // Child input redirection
        if (infile != NULL) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { 
                perror("Input redirection"); 
                _exit(1); 
            }
            dup2(fd, STDIN_FILENO); 
            close(fd);
        } 
        else if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
        }

        // Child output redirection
        if (outfile != NULL) {
            int flags = O_WRONLY | O_CREAT;
            if (append_mode) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }

            int fd = open(outfile, flags, 0644);
            if (fd < 0) { 
                perror("Output redirection"); 
                _exit(1); 
            }
            dup2(fd, STDOUT_FILENO); 
            close(fd);
        } 
        else if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
        }

        // Close inherited pipe descriptors inside child
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }

        // Reset signal handling for child process
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (execvp(clean_args[0], clean_args) < 0) {
            perror("Execution failed");
            _exit(1);
        }
    }

    *child_pid = pid;
    free(clean_args);
    free_token_array(tokens);
}

// Execute pipe commands split by |
void execute_pipe_sequence(char *pipe_line) {
    char *cmd = pipe_line;
    bool is_bg = false;

    // Check for trailing background job symbol
    char *bg_ptr = strchr(pipe_line, '&');
    if (bg_ptr != NULL) {
        is_bg = true;
        *bg_ptr = '\0';
    }

    int num_cmds = 0;
    char **commands = split_string(cmd, "|", &num_cmds);

    if (commands == NULL || num_cmds == 0) {
        free_token_array(commands);
        return;
    }

    int in_fd = STDIN_FILENO;
    int pipefds[2];
    pid_t pids[256];
    int pids_count = 0;

    for (int i = 0; i < num_cmds; i++) {
        int out_fd = STDOUT_FILENO;

        if (i < num_cmds - 1) {
            if (pipe(pipefds) < 0) {
                perror("pipe");
                break;
            }
            out_fd = pipefds[1];
        }

        pid_t child_pid = -1;
        execute_single_command_in_pipeline(commands[i], in_fd, out_fd, &child_pid);

        if (child_pid > 0) {
            pids[pids_count] = child_pid;
            pids_count++;
        }

        // Close pipe ends in parent process
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }

        if (i < num_cmds - 1) {
            in_fd = pipefds[0];
        }
    }

    // Wait for foreground commands or print background PID
    if (!is_bg) {
        for (int i = 0; i < pids_count; i++) {
            if (pids[i] > 0) {
                FOREGROUND_PID = pids[i];
                int status;
                waitpid(pids[i], &status, WUNTRACED);
                FOREGROUND_PID = -1;
            }
        }
    } 
    else {
        for (int i = 0; i < pids_count; i++) {
            if (pids[i] > 0) {
                printf("[%d]\n", pids[i]);
            }
        }
    }

    free_token_array(commands);
}

// Split input line by ; and process command sequences
void execute_command_line(char *line) {
    char *trimmed = trim_whitespace(line);
    if (strlen(trimmed) == 0) {
        return;
    }

    add_history(trimmed);

    int count = 0;
    char **semi_commands = split_string(trimmed, ";", &count);

    for (int i = 0; i < count; i++) {
        char *sub_cmd = trim_whitespace(semi_commands[i]);
        if (strlen(sub_cmd) > 0) {
            execute_pipe_sequence(sub_cmd);
        }
    }

    free_token_array(semi_commands);
}