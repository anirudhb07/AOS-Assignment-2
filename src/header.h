#ifndef HEADER_H
#define HEADER_H

#include <sys/types.h> //for data types like pid_t, mode_t, off_t, size_t
#include <sys/stat.h> //data structures and functions for file metadata and permissions and permissiojn bit flags
#include <sys/wait.h> //declarations for process control functions like waitpid and macros for interpreting process termination status
#include <fcntl.h> //file control options and flags for open, fcntl, and other file-related operations
#include <unistd.h> //for various system calls like read, write, close
#include <pwd.h> //for user information functions
#include <dirent.h> //for directory entry functions
#include <signal.h> //for signal handling functions
#include <termios.h> //for terminal I/O control functions
#include <stdio.h> //standard C I/O
#include <stdlib.h> // memory management, process control, conversions
#include <string.h> //string manipulation functions
#include <errno.h> //error number definitions
#include <time.h> //time and date functions

#define MAX_CMD_LEN 1024
#define MAX_HIST_SIZE 20

extern char HOME_DIR[1024]; //declare without defining
extern char PREV_DIR[1024];
extern pid_t FOREGROUND_PID;
extern struct termios orig_termios;

// Utility functions
void get_relative_path(const char *cwd, const char *home, char *out);
char* trim_whitespace(char *str);
char** split_string(const char *str, const char *delim, int *count);
void free_token_array(char **tokens);

// Shell Features
void display_prompt();
int execute_builtin(char **args);
int is_builtin(const char *cmd);
void handle_cd(char **args);
void handle_pwd();
void handle_echo(char **args);
void handle_ls(char **args);
void handle_pinfo(char **args);
void handle_search(char **args);
bool search_recursive(const char *dir_path, const char *target);

// History
void init_history();
void add_history(const char *cmd);
void display_history(int count);
char* get_history_at(int index);
int get_history_count();

// Execution & Redirection
void execute_command_line(char *line);
void execute_pipe_sequence(char *pipe_line);
void execute_single_command(char *cmd_str, int in_fd, int out_fd, bool is_bg);

// Raw Mode & Line Editing
void init_terminal_driver();
void reset_terminal_driver();
char* read_input_line();
void handle_tab_completion(char *buf, int *len);

// Signals
void setup_signal_handlers();

#endif // HEADER_H