#include "shell.h"

// Read the command from stdin
char* read_command() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    // Get the username
    char* username = getenv("USER");
    if (!username) {
        username = "user"; // Fallback if USER environment variable is not set
    }

    char prompt[1100];
    // Format with color: username@cwd $
// Format with color for Raspberry Pi: username@cwd $
    sprintf(prompt, "\001\033[1;32m\002%s\001\033[1;37m\002@\001\033[1;36m\002%s \001\033[1;33m\002$ \001\033[0m\002",
            username, cwd);
    char* command = readline(prompt);

    // If EOF, handle it
    if (!command) {
        printf("\n");
        exit(EXIT_SUCCESS);
    }

    // Add non-empty commands to history
    if (command[0] != '\0') {
        add_history(command);
    }

    return command;
}

// Parse the command into arguments
char** parse_command(char* command) {
    char** args = malloc((MAX_ARGS + 1) * sizeof(char*));
    if (!args) {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }

    int position = 0;
    int i = 0;
    int in_quotes = 0;
    char current_arg[MAX_COMMAND_LENGTH] = {0};
    int arg_pos = 0;

    while (command[i] != '\0' && position < MAX_ARGS) {
        // Handle quotes
        if (command[i] == '"') {
            in_quotes = !in_quotes;
            i++;
            continue;
        }

        // If space and not in quotes, end current argument
        if (command[i] == ' ' && !in_quotes) {
            if (arg_pos > 0) {
                current_arg[arg_pos] = '\0';
                args[position] = strdup(current_arg);
                position++;
                arg_pos = 0;
            }
        } else {
            // Handle environment variables
            if (command[i] == '$' && command[i+1] != '\0' && command[i+1] != ' ') {
                i++;
                char var_name[MAX_COMMAND_LENGTH];
                int var_pos = 0;

                // Extract variable name
                while (command[i] != '\0' && command[i] != ' ' && command[i] != '"' && var_pos < MAX_COMMAND_LENGTH - 1) {
                    var_name[var_pos++] = command[i++];
                }
                var_name[var_pos] = '\0';

                // Get environment variable value
                char* var_value = getenv(var_name);
                if (var_value) {
                    // Append variable value to current argument
                    int j = 0;
                    while (var_value[j] != '\0' && arg_pos < MAX_COMMAND_LENGTH - 1) {
                        current_arg[arg_pos++] = var_value[j++];
                    }
                }
                continue;
            } else {
                // Add character to current argument
                current_arg[arg_pos++] = command[i];
            }
        }
        i++;
    }

    // Add the last argument if there is one
    if (arg_pos > 0) {
        current_arg[arg_pos] = '\0';
        args[position] = strdup(current_arg);
        position++;
    }

    args[position] = NULL;  // Null-terminate the arguments array
    return args;
}

// Update free_args to free individual strings
void free_args(char** args) {
    if (!args) return;

    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);  // Free each string
    }
    free(args);  // Free the array
}

// Execute the command
int execute_command(char** args) {
    if (args[0] == NULL) {
        // Empty command
        return 1;
    }

    // Check for aliases
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(args[0], aliases[i].name) == 0) {
            // Parse the alias value into arguments
            char* alias_cmd = strdup(aliases[i].value);
            char** alias_args = parse_command(alias_cmd);

            // Skip built-in checking and go straight to execution
            pid_t pid = fork();

            if (pid == 0) {
                // Child process
                if (execvp(alias_args[0], alias_args) == -1) {
                    perror("command execution failed");
                    exit(EXIT_FAILURE);
                }
            } else if (pid < 0) {
                // Error forking
                perror("fork failed");
            } else {
                // Parent process
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }

            // Clean up
            free(alias_cmd);
            free_args(alias_args);
            return 1;
        }
    }

    // Search for built-in command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(args[0], commands[i].name) == 0) {
            return commands[i].func(args);
        }
    }

    // Fork a child process
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("command execution failed");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Error forking
        perror("fork failed");
    } else {
        // Parent process
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}