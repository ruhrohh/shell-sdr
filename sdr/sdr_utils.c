/**
 * @file sdr_utils.c
 * @brief Utility functions for SDR functionality
 *
 * Provides common utility functions for the SDR components, including:
 * - Directory creation for data storage
 * - Timestamp generation for filenames
 * - Device opening/closing helpers
 * - Terminal-based spectrum visualization
 * - Signal power calculation
 */

#include "shell.h"

// Callback function for async reads
static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
    if (ctx) {
        FILE *file = (FILE *)ctx;
        fwrite(buf, 1, len, file);
    }
}

// Create data directories if they don't exist
int create_data_directories() {
    struct stat st = {0};

    // Create main data directory
    if (stat(DATA_DIR, &st) == -1) {
        if (mkdir(DATA_DIR, 0700) == -1) {
            perror("Failed to create data directory");
            return 0;
        }
    }

    // Create spectrum logs directory
    if (stat(SPECTRUM_DIR, &st) == -1) {
        if (mkdir(SPECTRUM_DIR, 0700) == -1) {
            perror("Failed to create spectrum logs directory");
            return 0;
        }
    }

    // Create IQ samples directory
    if (stat(IQ_DIR, &st) == -1) {
        if (mkdir(IQ_DIR, 0700) == -1) {
            perror("Failed to create IQ samples directory");
            return 0;
        }
    }

    // Create SNR logs directory
    if (stat(SNR_DIR, &st) == -1) {
        if (mkdir(SNR_DIR, 0700) == -1) {
            perror("Failed to create SNR logs directory");
            return 0;
        }
    }

    return 1;
}

// Get formatted timestamp string for filenames
char* get_timestamp_string() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Allocate a much larger buffer to be safe
    char *timestamp = malloc(64);
    if (timestamp == NULL) {
        perror("Failed to allocate memory for timestamp");
        return NULL;
    }

    // Use snprintf instead of sprintf to prevent buffer overflow
    snprintf(timestamp, 64, "%04d-%02d-%02d_%02d-%02d-%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    return timestamp;
}

// Open SDR device with default settings
int open_sdr_device(rtlsdr_dev_t **dev) {
    int device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        fprintf(stderr, "No RTL-SDR devices found\n");
        return 0;
    }

    // Open first device
    if (rtlsdr_open(dev, 0) < 0) {
        fprintf(stderr, "Failed to open RTL-SDR device\n");
        return 0;
    }

    // Set default settings
    rtlsdr_set_sample_rate(*dev, DEFAULT_SAMPLE_RATE);
    rtlsdr_set_center_freq(*dev, DEFAULT_FREQ);
    rtlsdr_set_tuner_gain_mode(*dev, 0); // Auto gain

    return 1;
}

// Close SDR device
void close_sdr_device(rtlsdr_dev_t *dev) {
    if (dev) {
        rtlsdr_close(dev);
    }
}

// Helper function to find maximum power
double find_max_power(double* powers, int n_points) {
    double max = 0;
    for (int i = 0; i < n_points; i++) {
        if (powers[i] > max) max = powers[i];
    }
    return max;
}

// Function to display spectrum in terminal
int display_terminal_spectrum(uint32_t* freqs, double* powers, int n_points, uint32_t current_freq) {
    int viz_width = 80;
    int viz_height = 20;
    char spectrum_viz[20][81]; // +1 for null terminator

    // Find max power for scaling
    double max_power = find_max_power(powers, n_points);
    if (max_power <= 0) max_power = 1.0; // Avoid division by zero

    // Initialize visualization grid
    for (int y = 0; y < viz_height; y++) {
        for (int x = 0; x < viz_width; x++) {
            spectrum_viz[y][x] = ' ';
        }
        spectrum_viz[y][viz_width] = '\0';
    }

    // Plot spectrum data
    for (int i = 0; i < n_points; i++) {
        int x = (int)(((double)i / n_points) * viz_width);
        if (x >= viz_width) x = viz_width - 1;

        // Convert power to dB for better visualization
        double power_db = 10 * log10(powers[i] + 1e-10); // Avoid log(0)
        double min_db = -30; // Adjust based on your typical noise floor
        double normalized_power = (power_db - min_db) / (-min_db);
        if (normalized_power < 0) normalized_power = 0;
        if (normalized_power > 1) normalized_power = 1;

        int y = viz_height - (int)(normalized_power * viz_height) - 1;
        if (y < 0) y = 0;

        // Draw the bar
        for (int j = viz_height - 1; j >= y; j--) {
            spectrum_viz[j][x] = '#';
        }
    }

    // Clear screen and position cursor at top
    printf("\033[2J\033[H");

    // Draw frequency scale
    printf("Frequency (MHz): %.1f", freqs[0]/1e6);
    for (int i = 1; i < 8; i++) {
        int idx = (i * n_points) / 8;
        if (idx < n_points) {
            int pos = (int)(((double)idx / n_points) * (viz_width - 12));
            printf("%*s%.1f", pos, "", freqs[idx]/1e6);
        }
    }
    printf("\n");

    // Draw visualization
    for (int y = 0; y < viz_height; y++) {
        printf("%s\n", spectrum_viz[y]);
    }

    // Draw status line
    printf("Scanning: Currently at %.2f MHz | Progress: %.1f%%\n",
           current_freq/1e6, ((current_freq - freqs[0]) / (double)(freqs[n_points-1] - freqs[0])) * 100);

    return 0;
}