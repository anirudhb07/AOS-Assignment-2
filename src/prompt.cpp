#include "header.h"

void display_prompt() {
    char hostname[256];
    char cwd[1024];
    char relative_path[1024];

    // Get current logged-in username
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    char *username = (pw != NULL) ? pw->pw_name : (char*)"user";

    // Get system hostname
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "system");
    }

    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "unknown");
    }

    // Map path relative to launch directory
    get_relative_path(cwd, HOME_DIR, relative_path);

    // Print assignment required prompt format
    printf("<%s@%s:%s> ", username, hostname, relative_path);
    fflush(stdout);
}