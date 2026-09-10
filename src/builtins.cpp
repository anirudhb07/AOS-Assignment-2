#include "header.h"

// Check if a command matches one of the internal shell built-ins
int is_builtin(const char *cmd) {
    if (cmd == NULL) return 0;

    if (strcmp(cmd, "cd") == 0 || 
        strcmp(cmd, "pwd") == 0 ||
        strcmp(cmd, "echo") == 0 || 
        strcmp(cmd, "ls") == 0 ||
        strcmp(cmd, "pinfo") == 0 || 
        strcmp(cmd, "search") == 0 ||
        strcmp(cmd, "history") == 0 || 
        strcmp(cmd, "exit") == 0) {
        return 1;
    }

    return 0;
}

// Handle directory changes
void handle_cd(char **args) {
    int arg_count = 0;
    while (args[arg_count] != NULL) {
        arg_count++; //count total no of argument tokens passed to cd
    }

    // Specification: error if more than 1 argument is passed
    if (arg_count > 2) { //more than one target directory specified
        printf("Invalid arguments for error handling\n");
        return;
    }

    char current_cwd[1024]; //buffer to save current working directory before changing it
    //get absolute path of current working directory and store in current_cwd
    if (getcwd(current_cwd, sizeof(current_cwd)) == NULL) {
        perror("cd getcwd");
        return;
    }

    const char *target = NULL;

    // Default to home directory if no argument or ~ is passed
    if (arg_count == 1 || strcmp(args[1], "~") == 0) {
        target = HOME_DIR;
    } 
    else if (strcmp(args[1], "-") == 0) {
        if (strlen(PREV_DIR) == 0) {
            printf("cd: OLDPWD not set\n");
            return;
        }
        target = PREV_DIR; //set to prev working directory if it exists
        printf("%s\n", target); //prints target directory path
    } 
    else {
        target = args[1]; //sets target to the user-specified directory path
    }

    // Change directory and update OLDPWD tracking
    if (chdir(target) != 0) {
        perror("cd");
    } else {
        strcpy(PREV_DIR, current_cwd); //update PREV_DIR to the previous working directory after successful change
    }
}

// Print working directory
void handle_pwd() {
    char cwd[1024];
    //retrieve the absolute path of the current working directory and store it in cwd
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

// Print command arguments separated by spaces
void handle_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) { //skip "echo"
        printf("%s", args[i]); //print each argument token
        if (args[i + 1] != NULL) {
            printf(" "); //print space between arguments
        }
    }
    printf("\n"); //end with newline
}

// Helper to format ls -l file permissions and metadata
void print_ls_long(const char *path, const char *filename) {
    struct stat st; //for file metadata
    char full_path[2048];
    sprintf(full_path, "%s/%s", path, filename);//constructs the full path to the file by combining the directory path and filename

    if (lstat(full_path, &st) < 0) { //gets metadata
        return;
    }

    // File type flag
    if (S_ISDIR(st.st_mode)) {
        printf("d"); //print d for directories
    } else if (S_ISLNK(st.st_mode)) {
        printf("l"); //print l for symbolic links
    } else {
        printf("-"); //print - for regular files
    }

    // Permissions: User, Group, Others
    // bitwise test against r/w/ex flags
    // Owner permissions
    printf((st.st_mode & S_IRUSR) ? "r" : "-");
    printf((st.st_mode & S_IWUSR) ? "w" : "-");
    printf((st.st_mode & S_IXUSR) ? "x" : "-");

    // Group permissions
    printf((st.st_mode & S_IRGRP) ? "r" : "-");
    printf((st.st_mode & S_IWGRP) ? "w" : "-");
    printf((st.st_mode & S_IXGRP) ? "x" : "-");

    // Other permissions
    printf((st.st_mode & S_IROTH) ? "r" : "-");
    printf((st.st_mode & S_IWOTH) ? "w" : "-");
    printf((st.st_mode & S_IXOTH) ? "x" : "-");

    // Links, Owner, Size, Time
    printf(" %2ld", (long)st.st_nlink); //hard link count

    //converts user id int into username string
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw != NULL) {
        printf(" %s", pw->pw_name); //print username of file owner
    } else {
        printf(" user");
    }

    printf(" %ld", (long)st.st_size); //print file size in bytes

    char timebuf[64];
    struct tm *tm = localtime(&st.st_mtime); //last modification time
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);
    printf(" %s", timebuf); //print the formatted modification time

    printf(" %s\n", filename); //print filename at the end of the line
}

// Custom ls implementation supporting -a, -l, -la, and directories
void handle_ls(char **args) {
    bool flag_a = false; //show hidden files
    bool flag_l = false; //long listing
    char *dirs[256];
    int dir_count = 0;

    // Parse flags and folder targets
    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-' && strlen(args[i]) > 1) {
            for (size_t j = 1; j < strlen(args[i]); j++) {
                if (args[i][j] == 'a') flag_a = true;
                if (args[i][j] == 'l') flag_l = true;
            }
        } else { //non flag argument, treat as directory target
            dirs[dir_count] = args[i];
            dir_count++;
        }
    }

    // Default to current directory if no dir argument passed
    if (dir_count == 0) {
        dirs[0] = (char*)"."; // default to current directory
        dir_count = 1;
    }

    for (int d = 0; d < dir_count; d++) {
        const char *target = dirs[d];
        if (strcmp(target, "~") == 0) {
            target = HOME_DIR; //home directory if ~ is passed as argument
        }

        DIR *dir = opendir(target); //open target directory stream
        if (dir == NULL) {
            perror("ls");
            continue;
        }

        if (dir_count > 1) {
            printf("%s:\n", dirs[d]); //print directory name if multiple directories are listed
        }

        struct dirent *entry; //entry in direcotry stream
        while ((entry = readdir(dir)) != NULL) { //iterate through each entry in the directory stream
            // Skip hidden files if -a is not passed
            if (!flag_a && entry->d_name[0] == '.') {
                continue;
            }

            if (flag_l) { //print long listing format if -l is passed
                print_ls_long(target, entry->d_name);
            } else { //else print filenames line by line
                printf("%s\n", entry->d_name);
            }
        }

        closedir(dir); //close the directory stream after processing all entries

        if (d + 1 < dir_count) {
            printf("\n"); //print newline between multiple directory listings
        }
    }
}

// Recursive file/folder search helper
bool search_recursive(const char *dir_path, const char *target) {
    DIR *dir = opendir(dir_path); //open dir_path
    if (dir == NULL) return false;

    struct dirent *entry;
    bool found = false;

    while ((entry = readdir(dir)) != NULL) {
        // Skip parent and current directory links
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // If the current entry matches the target, we found it
        if (strcmp(entry->d_name, target) == 0) {
            closedir(dir);
            return true;
        }

        char subpath[2048];
        
        sprintf(subpath, "%s/%s", dir_path, entry->d_name);

        //if item is a directory, recursively search inside it
        //by constructing the full path to the subdirectory and calling search_recursive() on it
        struct stat st;
        if (stat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (search_recursive(subpath, target)) {
                found = true;
                break;
            }
        }
    }

    closedir(dir);
    return found;
}

// Search built-in entry point
void handle_search(char **args) {
    if (args[1] == NULL) {
        printf("search: missing argument\n");
        return;
    }

    // calls search_recursive() starting from the current directory (".")
    // to look for the target specified in args[1]
    if (search_recursive(".", args[1])) {
        printf("True\n");
    } else {
        printf("False\n");
    }
}

// Dispatch built-in commands
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        handle_cd(args);
        return 1;
    }
    if (strcmp(args[0], "pwd") == 0) {
        handle_pwd();
        return 1;
    }
    if (strcmp(args[0], "echo") == 0) {
        handle_echo(args);
        return 1;
    }
    if (strcmp(args[0], "ls") == 0) {
        handle_ls(args);
        return 1;
    }
    if (strcmp(args[0], "pinfo") == 0) {
        handle_pinfo(args);
        return 1;
    }
    if (strcmp(args[0], "search") == 0) {
        handle_search(args);
        return 1;
    }
    if (strcmp(args[0], "history") == 0) {
        int count = 10;
        if (args[1] != NULL) {
            count = atoi(args[1]);
        }
        display_history(count);
        return 1;
    }

    return 0;
}