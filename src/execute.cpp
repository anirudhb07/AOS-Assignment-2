#include "header.h"

pid_t FOREGROUND_PID = -1;

// Helper to execute a single command within a pipeline stage
void execute_single_command_in_pipeline(char *cmd_str, int in_fd, int out_fd, pid_t *child_pid) {
    int token_count = 0;
    //Breaks the string into space/tab tokens.
    char **tokens = split_string(cmd_str, " \t", &token_count);

    if (tokens == NULL || token_count == 0) {
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    char *infile = NULL; // pointers to token array elements for input/output redirection
    char *outfile = NULL;
    bool append_mode = false;

    char **clean_args = (char**)malloc((token_count + 1) * sizeof(char*));
    int clean_count = 0;

    // Parse input/output redirection symbols and build clean argument array
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "<") == 0) { //read from file
            if (tokens[i + 1] != NULL) {
                infile = tokens[i + 1];
                i++;
            }
        } 
        else if (strcmp(tokens[i], ">") == 0) { //overwrite to file
            if (tokens[i + 1] != NULL) {
                outfile = tokens[i + 1];
                append_mode = false;
                i++;
            }
        } 
        else if (strcmp(tokens[i], ">>") == 0) { //append to file
            if (tokens[i + 1] != NULL) {
                outfile = tokens[i + 1];
                append_mode = true; //append mode
                i++;
            }
        } 
        else {
            clean_args[clean_count] = tokens[i];
            clean_count++;
        }
    }
    clean_args[clean_count] = NULL;

    if (clean_args[0] == NULL) { // No command to execute after parsing
        free(clean_args);
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    // Handle internal built-in execution in parent process
    if (is_builtin(clean_args[0])) {
        int saved_stdin = dup(STDIN_FILENO); //dup() duplicates the file descriptor
        int saved_stdout = dup(STDOUT_FILENO); //because redirection will temporarily overwrite standard I/O

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
            dup2(fd, STDIN_FILENO); //so the built-in reads from the opened file instead of the keyboard.
            close(fd);
        } 
        else if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO); //Handles pipe input redirection
        }

        // Output redirection setup
        if (outfile != NULL) {
            // Set up file flags for output redirection
            int flags = O_WRONLY | O_CREAT; //file flags: Write-Only mode and Create file
            if (append_mode) {
                flags |= O_APPEND; //append mode: write data to the end of the file
            } else {
                flags |= O_TRUNC; //truncate mode: clear the file before writing
            }

            int fd = open(outfile, flags, 0644); //open the file with the specified flags and permissions (0644)
            if (fd < 0) { 
                perror("Output redirection"); 
                free(clean_args); 
                free_token_array(tokens); 
                *child_pid = -1;
                return; 
            }
            dup2(fd, STDOUT_FILENO); //so command output goes to the file instead of the terminal display.
            close(fd);
        } 
        else if (out_fd != STDOUT_FILENO) { //Handles pipe output redirection
            dup2(out_fd, STDOUT_FILENO);
        }

        // Execute the built-in command
        execute_builtin(clean_args);

        // Restore standard I/O descriptors
        dup2(saved_stdin, STDIN_FILENO); //Restores back to its original state
        close(saved_stdin); //close duplicated file descriptor after restoring
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);

        free(clean_args);
        free_token_array(tokens);
        *child_pid = -1;
        return;
    }

    // System binary execution using fork and execvp
    // for external binary programs located in system directories
    pid_t pid = fork();

    // In the child process, set up redirection and execute the command
    if (pid == 0) {
        // makes the child the leader of a new process group (its own pgid == its own pid).
        setpgid(0, 0);//Prevents terminal signals like SIGINT (Ctrl+C) sent to the parent shell from automatically killing child background processes

        // Child input redirection
        if (infile != NULL) {
            int fd = open(infile, O_RDONLY);
            if (fd < 0) { 
                perror("Input redirection"); 
                _exit(1); 
            }
            // so the child process reads from the opened file instead of the keyboard.
            // This is done by duplicating the file descriptor to standard input.
            dup2(fd, STDIN_FILENO); 
            close(fd);
        } 
        else if (in_fd != STDIN_FILENO) {
            // If reading from a pipe instead of a file, Handles pipe input redirection
            dup2(in_fd, STDIN_FILENO);
        }

        // Child output redirection
        if (outfile != NULL) {
            int flags = O_WRONLY | O_CREAT; //file flags: Write-Only mode and Create file
            if (append_mode) {
                flags |= O_APPEND; //append mode: write data to the end of the file
            } else {
                flags |= O_TRUNC; //truncate mode: clear the file before writing
            }



            int fd = open(outfile, flags, 0644);
            if (fd < 0) { 
                perror("Output redirection"); 
                _exit(1); 
            }
            dup2(fd, STDOUT_FILENO); //Redirects standard output to the opened output file. 
            close(fd);
        } 
        else if (out_fd != STDOUT_FILENO) { //If piping output to another command
            dup2(out_fd, STDOUT_FILENO); //redirects STDOUT_FILENO to the pipe's write descriptor
        }

        // Close inherited pipe descriptors inside child
        // so the child doesn't hold extra open pipe ends it shouldn't have,
        // which could prevent the parent from detecting EOF on the pipe.

        //Unclosed pipe descriptors leave reference counts open,
        //preventing downstream pipeline commands from receiving EOF (End Of File) signals
        // and hanging indefinitely.

        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }

        // Reset signal handling for child process
        // If the parent shell ignores these signals child processes inherit that behavior unless
        // explicitly reset to standard system defaults.
        //so external programs respond to keyboard interrupts naturally.
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);


        //Replaces the child process memory space with the new program specified by clean_args[0]
        if (execvp(clean_args[0], clean_args) < 0) {
            perror("Execution failed");
            _exit(1);
        }
    }

    //Stores the child's PID in the output parameter so the parent shell can track or waitpid() on it.
    *child_pid = pid;
    free(clean_args);
    free_token_array(tokens);
}

// Execute pipe commands split by |
void execute_pipe_sequence(char *pipe_line) {
    char *cmd = pipe_line; //Initializes a pointer to the command input string.
    bool is_bg = false; //Sets a flag to track whether the command should run in the background.

    // Check for trailing background job symbol
    char *bg_ptr = strchr(pipe_line, '&'); //Searches for the & symbol in the input command string
    if (bg_ptr != NULL) { //If & exists
        is_bg = true; //is_bg = true
        *bg_ptr = '\0'; //replaces & with a null terminator (\0), stripping it from the command arguments before processing.
    }

    int num_cmds = 0;
    // Tokenizes the command string using | as a delimiter,
    // returning an array of string commands (commands) and updating num_cmds

    //Splits the sub-command by | into pipeline stages
    char **commands = split_string(cmd, "|", &num_cmds);


    if (commands == NULL || num_cmds == 0) {
        free_token_array(commands);
        return;
    }

    int in_fd = STDIN_FILENO; //Tracks the input file descriptor for the current stage. Defaults to keyboard input (0) for the first command.
    int pipefds[2]; //Array to hold the read (pipefds[0]) and write (pipefds[1]) file descriptors returned by pipe()
    
    //Array and counter to store PIDs of spawned child processes so the shell can track or wait on them
    pid_t pids[256];
    int pids_count = 0;

    for (int i = 0; i < num_cmds; i++) {
        int out_fd = STDOUT_FILENO; //Defaults output for the command to standard terminal

        if (i < num_cmds - 1) { // If this is not the last command in the pipeline
            //Creates a unidirectional pipe channel. pipefds[1] is for writing,
            // and pipefds[0] is for reading.
            if (pipe(pipefds) < 0) {
                perror("pipe");
                break;
            }
            out_fd = pipefds[1]; //Sets the output stream of the current command to the write end of the newly created pipe.
        }
        // Close the previous pipe's read end in the parent process
        if (i > 0 && in_fd != STDIN_FILENO) {
            close(in_fd);
        }

        pid_t child_pid = -1;
        //Executes the command stage, binding input to in_fd and output to out_fd.
        // Passes &child_pid to store the created PID.
        execute_single_command_in_pipeline(commands[i], in_fd, out_fd, &child_pid);

        if (child_pid > 0) {
            pids[pids_count] = child_pid; //record child PID into tracking array
            pids_count++;
        }

        // Close pipe ends in parent process
        // If the parent keeps write descriptors open,
        //downstream commands in the pipe will never receive an EOF signal and will hang indefinitely.
        if (in_fd != STDIN_FILENO) {
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO) {
            close(out_fd);
        }

        if (i < num_cmds - 1) {
            in_fd = pipefds[0]; //Sets the input for the next command in the pipeline to the read end of the current pipe.
        }
    }

    // Wait for foreground commands or print background PID
    if (!is_bg) { //Foreground execution
        for (int i = 0; i < pids_count; i++) {
            if (pids[i] > 0) {
                FOREGROUND_PID = pids[i]; //Sets global state so signal handlers (like SIGINT / SIGTSTP) know which process to send signals to.
                int status;
                waitpid(pids[i], &status, WUNTRACED); //to block parent execution until that child finishes or stops.
                FOREGROUND_PID = -1; // Resets FOREGROUND_PID = -1 when done.
            }
        }
    } 
    else { //Background execution
        for (int i = 0; i < pids_count; i++) { //Skips waitpid(), allowing the parent shell to immediately return to the prompt.
            if (pids[i] > 0) {
                // Print the PID of each background process
                printf("[%d]\n", pids[i]);
            }
        }
    } 

    free_token_array(commands);
}

// Split input line by ; and process command sequences
void execute_command_line(char *line) {
    //Removes leading and trailing whitespace chars from the user's input string.
    char *trimmed = trim_whitespace(line);
    if (strlen(trimmed) == 0) {
        return;
    }

    add_history(trimmed); //Saves the valid, non-empty command string to the shell's command history buffer

    int count = 0; //to store the total number of semicolon-separated sub-commands parsed

    //Splits the input string on ; delimiters into an array of individual command strings
    char **semi_commands = split_string(trimmed, ";", &count);

    for (int i = 0; i < count; i++) {
        char *sub_cmd = trim_whitespace(semi_commands[i]);
        if (strlen(sub_cmd) > 0) {
            execute_pipe_sequence(sub_cmd); //Passes each command to the pipeline execution function
        }
    }

    free_token_array(semi_commands);
}