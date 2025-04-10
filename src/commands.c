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

// Define the commands array with added SDR commands
shell_command commands[] = {
    // Original commands
    {"cd", cmd_cd, "Change directory"},
    {"exit", cmd_exit, "Exit the shell"},
    {"hello", cmd_hello, "Print a greeting"},
    {"help", cmd_help, "Display this help information"},

    // New SDR commands
    {"sdr_info", cmd_sdr_info, "Display RTL-SDR device information"},
    {"sdr_scan", cmd_sdr_scan, "Scan frequency range - usage: sdr_scan [start_freq] [end_freq] [step] [samples]"},
    {"sdr_monitor", cmd_sdr_monitor, "Monitor signal level at frequency - usage: sdr_monitor [frequency]"},
    {"sdr_record", cmd_sdr_record, "Record IQ data samples - usage: sdr_record [frequency] [duration]"},
    {"sdr_snr", cmd_sdr_snr, "Measure signal-to-noise ratio - usage: sdr_snr [frequency] [duration]"},

    {"upload_discord", cmd_upload_discord, "Upload SDR files to Discord - usage: upload_discord [file_type]"},

    {NULL, NULL, NULL}
};