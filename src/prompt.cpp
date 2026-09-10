#include "header.h"

void display_prompt() {
    char hostname[256]; //tore the system's network hostname.
    char cwd[1024]; //Buffer to hold the absolute path of the current working directory.
    char relative_path[1024]; //Buffer to store the formatted relative path

    // Get current logged-in username
    uid_t uid = geteuid(); //Retrieves the Effective User ID (UID) of the process running the shell
    struct passwd *pw = getpwuid(uid); //Queries the system password database for metadata matching uid
    char *username = (pw != NULL) ? pw->pw_name : (char*)"user";

    // Get system hostname
    //Fetches the machine's domain/system hostname string
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
    //Flushes the output buffer immediately to guarantee that the prompt appears on screen
    // before reading user input, avoiding buffer delays.
    fflush(stdout);
}