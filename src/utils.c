#include "shell.h"

// Add this function to get built-in commands
char *builtin_commands[] = {
    "cd",
    "exit",
    NULL
};

// Generate completion matches
// Update your generator function to handle both commands and filenames
char* myshell_generator(const char* text, int state) {
    static int list_index, len;
    static int is_command_completion;
    static DIR *dir;
    static struct dirent *entry;
    static char *directory;

    // First time called for this completion
    if (!state) {
        list_index = 0;
        len = strlen(text);
        is_command_completion = (rl_line_buffer[0] == '\0' ||
                                 strchr(rl_line_buffer, ' ') == NULL);

        // If we're completing a command (first word)
        if (is_command_completion) {
            // We'll start with built-in commands
            return NULL;  // Will trigger next state
        }

        // For file completion, determine if we're dealing with a path
        directory = ".";  // Default to current directory
        char *last_slash = strrchr(text, '/');

        if (last_slash) {
            // We have a path - extract the directory part
            int dir_len = last_slash - text + 1;
            directory = malloc(dir_len + 1);
            strncpy(directory, text, dir_len);
            directory[dir_len] = '\0';

            // Adjust text to point just to the filename part
            text = last_slash + 1;
            len = strlen(text);
        }

        // Open directory
        dir = opendir(directory);
        if (!dir) {
            if (directory != ".") free(directory);
            return NULL;
        }
    }

    // Check built-in commands first
    if (is_command_completion) {
        while (builtin_commands[list_index] != NULL) {
            char *name = builtin_commands[list_index++];
            if (strncmp(name, text, len) == 0) {
                return strdup(name);
            }
        }

        // Switch to checking PATH for executables
        is_command_completion = 0;
        directory = ".";
        dir = opendir(directory);
        if (!dir) return NULL;

        // Fall through to filename completion
    }

    // Return the next match from files
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. if at the start of completion
        if (text[0] == '\0' &&
            (strcmp(entry->d_name, ".") == 0 ||
             strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        if (strncmp(entry->d_name, text, len) == 0) {
            char *name;

            if (directory != ".") {
                // Need to prepend the directory path
                name = malloc(strlen(directory) + strlen(entry->d_name) + 1);
                strcpy(name, directory);
                strcat(name, entry->d_name);
            } else {
                name = strdup(entry->d_name);
            }

            // If it's a directory, add a slash
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s%s",
                     directory, entry->d_name);

            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char *with_slash = malloc(strlen(name) + 2);
                strcpy(with_slash, name);
                strcat(with_slash, "/");
                free(name);
                name = with_slash;
            }

            return name;
        }
    }

    // No more matches
    closedir(dir);
    if (directory != "." && directory != NULL) {
        free(directory);
    }
    return NULL;
}

// Custom completion function
char** myshell_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, myshell_generator);
}