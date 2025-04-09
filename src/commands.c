#include "shell.h"

// Built-in command implementations
int cmd_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: expected argument\n");
    } else if (chdir(args[1]) != 0) {
        perror("cd failed");
    }
    return 1;
}

int cmd_exit(char **args) {
    return 0;  // Return 0 to signal exit
}

int cmd_hello(char **args) {
    if (args[1] == NULL) {
        printf("Hello, world!\n");
    } else {
        printf("Hello, %s!\n", args[1]);
    }
    return 1;
}

int cmd_help(char **args) {
    printf("MyShell built-in commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %s\t%s\n", commands[i].name, commands[i].help);
    }
    return 1;
}

// Define the commands array
shell_command commands[] = {
    {"cd", cmd_cd, "Change directory"},
    {"exit", cmd_exit, "Exit the shell"},
    {"hello", cmd_hello, "Print a greeting"},
    {"help", cmd_help, "Display this help information"},
    {NULL, NULL, NULL}
};