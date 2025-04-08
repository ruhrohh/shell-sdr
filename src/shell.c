// Main shell loop

#include "shell.h"

int main() {
    char* command;
    char** args;
    int status = 1;

    // Configure readline
    rl_attempted_completion_function = myshell_completion;
    // Tell readline to use our custom tab completion
    rl_bind_key('\t', rl_complete);

    // Initialize history
    using_history();
    read_history(".myshell_history");

    printf("Welcome to MyShell! Type 'exit' to quit.\n");

    while(status) {
        command = read_command();
        args = parse_command(command);
        status = execute_command(args);

        // Clean up
        free(command);
        free_args(args);
    }

    // Save history on exit
    write_history(".myshell_history");

    printf("Goodbye!\n");
    return 0;
}