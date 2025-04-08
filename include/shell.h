#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>  // For PATH_MAX

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

// Function declarations
void print_prompt();
char* read_command();
char** parse_command(char* command);
int execute_command(char** args);
void free_args(char** args);
char** myshell_completion(const char* text, int start, int end);
char* myshell_generator(const char* text, int state);

#endif //SHELL_H
