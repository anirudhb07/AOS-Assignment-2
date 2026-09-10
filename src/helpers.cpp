#include "header.h"

char HOME_DIR[1024] = "";
char PREV_DIR[1024] = "";

// Trim leading and trailing spaces/tabs
char* trim_whitespace(char *str) {
    if (!str) return NULL;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    if (*str == '\0') return str;

    // Find end of string and trim trailing whitespace
    int len = strlen(str);
    char *end = str + len - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    end[1] = '\0';

    return str;
}

// Map current path to ~ if inside home directory
void get_relative_path(const char *cwd, const char *home, char *out) {
    int home_len = strlen(home);

    // If cwd starts with home path
    // and is either exact match (\0) or a subfolder boundary (/).
    if (strncmp(cwd, home, home_len) == 0 && (cwd[home_len] == '/' || cwd[home_len] == '\0')) {
        out[0] = '~';
        out[1] = '\0';
        strcat(out, cwd + home_len); //Appends path remainder after home directory prefix
    } else {
        strcpy(out, cwd); //If path lies outside home, copies full absolute path into out.
    }
}

// Split string by delimiter into array of tokens
char** split_string(const char *str, const char *delim, int *count) {
    int capacity = 10; //initial capacity for token array
    int num_tokens = 0;
    char **tokens = (char**)malloc(capacity * sizeof(char*)); //memory block for 10 char* elements

    char *copy = strdup(str); //Creates a duplicate string buffer
    char *token = strtok(copy, delim); //Extracts the first token bounded by delim

    while (token != NULL) {
        if (num_tokens + 1 >= capacity) {
            capacity *= 2; //dynamic resizing if capacity limit is reached
            tokens = (char**)realloc(tokens, capacity * sizeof(char*));
        }
        tokens[num_tokens] = strdup(token); //Allocates new heap copy for current token and saves it in array.
        num_tokens++;
        token = strtok(NULL, delim); //Requests next token from strtok.
    }

    tokens[num_tokens] = NULL; // Null terminate for execvp
    free(copy);

    if (count != NULL) {
        *count = num_tokens;
    }

    return tokens;
}

// Free memory allocated for tokens
void free_token_array(char **tokens) {
    if (tokens == NULL) return;

    //freeing each individually heap-allocated token string
    for (int i = 0; tokens[i] != NULL; i++) { 
        free(tokens[i]);
    }
    free(tokens);
}