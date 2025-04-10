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

int cmd_upload_discord(char **args) {
    char command[512];

    // Create data directories if they don't exist
    if (!create_data_directories()) {
        return 1;
    }

    if (args[1] && strcmp(args[1], "spectrum") == 0) {
        // Only upload spectrum files
        printf("Uploading spectrum files to Discord...\n");
        system("python3 discord_uploader.py spectrum");
    } else if (args[1] && strcmp(args[1], "iq") == 0) {
        // Only upload IQ files
        printf("Uploading IQ files to Discord...\n");
        system("python3 discord_uploader.py iq");
    } else if (args[1] && strcmp(args[1], "snr") == 0) {
        // Only upload SNR files
        printf("Uploading SNR files to Discord...\n");
        system("python3 discord_uploader.py snr");
    } else {
        // Upload all file types
        printf("Uploading all SDR files to Discord...\n");
        system("python3 discord_uploader.py all");
    }

    return 1;
}

int cmd_alias(char **args) {
    // No arguments - list all aliases
    if (args[1] == NULL) {
        for (int i = 0; i < alias_count; i++) {
            printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
        }
        return 1;
    }

    // Check if this is a simple alias definition (name=value)
    char *equals = strchr(args[1], '=');
    if (equals) {
        // Setting an alias like: alias ls=ls --color
        *equals = '\0'; // Split at equals sign
        char *name = args[1];
        char *value = equals + 1;

        // If value is empty, check if there are more arguments
        if (*value == '\0' && args[2] != NULL) {
            // Build value from remaining arguments
            char full_value[1024] = "";
            for (int i = 2; args[i] != NULL; i++) {
                strcat(full_value, args[i]);
                if (args[i+1] != NULL) {
                    strcat(full_value, " ");
                }
            }
            value = full_value;
        }

        // Remove quotes if present
        if (value[0] == '\'' || value[0] == '"') {
            value++;
            char *end = value + strlen(value) - 1;
            if (*end == '\'' || *end == '"') {
                *end = '\0';
            }
        }

        // Check if alias already exists
        for (int i = 0; i < alias_count; i++) {
            if (strcmp(aliases[i].name, name) == 0) {
                // Update existing alias
                free(aliases[i].value);
                aliases[i].value = strdup(value);
                return 1;
            }
        }

        // Add new alias
        if (alias_count < MAX_ALIASES) {
            aliases[alias_count].name = strdup(name);
            aliases[alias_count].value = strdup(value);
            alias_count++;
        } else {
            fprintf(stderr, "Maximum number of aliases reached\n");
        }
    }
    // Handle format: alias name value
    else if (args[1] != NULL && args[2] != NULL) {
        char *name = args[1];

        // Build value from remaining arguments
        char value[1024] = "";
        for (int i = 2; args[i] != NULL; i++) {
            strcat(value, args[i]);
            if (args[i+1] != NULL) {
                strcat(value, " ");
            }
        }

        // Check if alias already exists
        for (int i = 0; i < alias_count; i++) {
            if (strcmp(aliases[i].name, name) == 0) {
                // Update existing alias
                free(aliases[i].value);
                aliases[i].value = strdup(value);
                return 1;
            }
        }

        // Add new alias
        if (alias_count < MAX_ALIASES) {
            aliases[alias_count].name = strdup(name);
            aliases[alias_count].value = strdup(value);
            alias_count++;
        } else {
            fprintf(stderr, "Maximum number of aliases reached\n");
        }
    }
    else {
        fprintf(stderr, "Usage: alias [name=value] or alias [name value]\n");
    }

    return 1;
}

int cmd_unalias(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "unalias: missing argument\n");
        return 1;
    }

    // Look for the alias to remove
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, args[1]) == 0) {
            // Free memory
            free(aliases[i].name);
            free(aliases[i].value);

            // Shift remaining aliases down
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j].name = aliases[j+1].name;
                aliases[j].value = aliases[j+1].value;
            }

            alias_count--;
            printf("Alias '%s' removed\n", args[1]);
            return 1;
        }
    }

    fprintf(stderr, "unalias: %s not found\n", args[1]);
    return 1;
}

// Define the commands array with added SDR commands
shell_command commands[] = {
    // Original commands
    {"cd", cmd_cd, "Change directory"},
    {"exit", cmd_exit, "Exit the shell"},
    {"hello", cmd_hello, "Print a greeting"},
    {"help", cmd_help, "Display this help information"},
    {"upload_discord", cmd_upload_discord, "Upload SDR files to Discord - usage: upload_discord [file_type]"},
    {"alias", cmd_alias, "Define or display aliases"},
    {"unalias", cmd_unalias, "Remove an alias"},

    // New SDR commands
    {"sdr_info", cmd_sdr_info, "Display RTL-SDR device information"},
    {"sdr_scan", cmd_sdr_scan, "Scan frequency range - usage: sdr_scan [start_freq] [end_freq] [step] [samples]"},
    {"sdr_monitor", cmd_sdr_monitor, "Monitor signal level at frequency - usage: sdr_monitor [frequency]"},
    {"sdr_record", cmd_sdr_record, "Record IQ data samples - usage: sdr_record [frequency] [duration]"},
    {"sdr_snr", cmd_sdr_snr, "Measure signal-to-noise ratio - usage: sdr_snr [frequency] [duration]"},

    {NULL, NULL, NULL}
};