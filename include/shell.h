/**
 * @file shell.h
 * @brief Main header file for the shell
 *
 * Defines constants, structures, and function prototypes for the entire
 * shell application. Includes declarations for both core shell functionality
 * and SDR components. This file is included by all source files.
 */

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
#include <time.h>    // For timestamps in SDR logs
#include <math.h>    // For log10f function
#include <rtl-sdr.h>  // RTL-SDR library
#include <fftw3.h>    // For FFT processing

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

// SDR Constants
#define DEFAULT_SAMPLE_RATE 2048000
#define DEFAULT_BUFFER_SIZE 16384
#define DEFAULT_FREQ 100000000  // 100 MHz
#define DATA_DIR "./data"
#define SPECTRUM_DIR "./data/spectrum_logs"
#define IQ_DIR "./data/iq_samples"
#define SNR_DIR "./data/snr_logs"

// New command structure
typedef int (*command_function)(char**);

typedef struct {
    char *name;
    command_function func;
    char *help;
} shell_command;

typedef struct {
    char *name;
    char *value;
} alias_t;

#define MAX_ALIASES 100
extern alias_t aliases[MAX_ALIASES];
extern int alias_count;

// Declare the commands array as extern
extern shell_command commands[];

// Function declarations
void print_prompt();
char* read_command();
char** parse_command(char* command);
int execute_command(char** args);
void free_args(char** args);
char** myshell_completion(const char* text, int start, int end);
char* myshell_generator(const char* text, int state);

// Built-in command functions
int cmd_cd(char **args);
int cmd_exit(char **args);
int cmd_help(char **args);
int cmd_hello(char **args);
int cmd_alias(char **args);
int cmd_unalias(char **args);

// SDR command functions
int cmd_sdr_scan(char **args);
int cmd_sdr_monitor(char **args);
int cmd_sdr_record(char **args);
int cmd_sdr_info(char **args);
int cmd_sdr_snr(char **args);

// SDR utility functions
int create_data_directories();
char* get_timestamp_string();
int open_sdr_device(rtlsdr_dev_t **dev);
void close_sdr_device(rtlsdr_dev_t *dev);
int display_terminal_spectrum(uint32_t* freqs, double* powers, int n_points, uint32_t current_freq);
double find_max_power(double* powers, int n_points);

// Configuration functions
char* get_config_file_path();
int initialize_config_file();
int load_aliases_from_config();
int save_aliases_to_config();
int alias_exists(const char* name);

#endif //SHELL_H