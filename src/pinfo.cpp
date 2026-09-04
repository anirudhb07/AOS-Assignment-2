#include "header.h"

// Display process information from the /proc filesystem
void handle_pinfo(char **args) {
    pid_t pid = getpid(); // Default to shell PID if no argument passed

    if (args[1] != NULL) {
        pid = atoi(args[1]);
    }

    char path[512];
    
    // 1. Read process state from /proc/<pid>/status
    sprintf(path, "/proc/%d/status", pid);
    FILE *status_file = fopen(path, "r");
    if (status_file == NULL) {
        printf("pinfo: Process %d does not exist\n", pid);
        return;
    }

    char line[256];
    char state_str[32] = "";
    
    while (fgets(line, sizeof(line), status_file) != NULL) {
        if (strncmp(line, "State:", 6) == 0) {
            sscanf(line + 6, "%s", state_str);
            break;
        }
    }
    fclose(status_file);

    // 2. Read virtual memory usage from /proc/<pid>/statm
    sprintf(path, "/proc/%d/statm", pid);
    FILE *statm_file = fopen(path, "r");
    char vm_size[64] = "0";

    if (statm_file != NULL) {
        if (fscanf(statm_file, "%s", vm_size) != 1) {
            strcpy(vm_size, "0");
        }
        fclose(statm_file);
    }

    // 3. Append '+' to status if process is in the foreground group
    pid_t foreground_pgid = tcgetpgrp(STDIN_FILENO);
    pid_t process_pgid = getpgid(pid);

    if (foreground_pgid == process_pgid && foreground_pgid != -1) {
        strcat(state_str, "+");
    }

    // 4. Resolve absolute/relative executable path from /proc/<pid>/exe
    sprintf(path, "/proc/%d/exe", pid);
    char exe_path[1024] = "";
    ssize_t link_len = readlink(path, exe_path, sizeof(exe_path) - 1);

    if (link_len != -1) {
        exe_path[link_len] = '\0';
        char relative_exe[1024];
        get_relative_path(exe_path, HOME_DIR, relative_exe);
        strcpy(exe_path, relative_exe);
    } else {
        strcpy(exe_path, "Executable Path not found");
    }

    // Output process information according to assignment specs
    printf("pid -- %d\n", pid);
    printf("Process Status -- %s\n", state_str);
    printf("memory -- %s {Virtual Memory}\n", vm_size);
    printf("Executable Path -- %s\n", exe_path);
}