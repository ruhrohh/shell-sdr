// Main shell loop

#include "shell.h"

// After includes, before main()
alias_t aliases[MAX_ALIASES];
int alias_count = 0;

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

    // Initialize default aliases
    aliases[alias_count].name = strdup("ls");
    aliases[alias_count].value = strdup("ls --color=auto");
    alias_count++;
    aliases[alias_count].name = strdup("congq");
    aliases[alias_count].value = strdup("nc localhost 7356");
    alias_count++;

    // Initialize and load config file
    initialize_config_file();
    load_aliases_from_config();

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

    // Save aliases to config file
    save_aliases_to_config();

    // Free aliases
    for (int i = 0; i < alias_count; i++) {
        free(aliases[i].name);
        free(aliases[i].value);
    }

    printf("Goodbye!\n");
    return 0;
}