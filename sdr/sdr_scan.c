#include "shell.h"

// Command to scan frequency range and log spectrum data
int cmd_sdr_scan(char **args) {
    if (!create_data_directories()) {
        return 1;
    }

    uint32_t start_freq = DEFAULT_FREQ;
    uint32_t end_freq = DEFAULT_FREQ + 10000000; // Default: 10 MHz range
    uint32_t step = 100000; // 100 kHz steps
    int samples = 10; // Number of samples per frequency

    // Add visualization flag
    int terminal_viz = 0;

    // Parse command arguments
    if (args[1]) start_freq = atoi(args[1]);
    if (args[2]) end_freq = atoi(args[2]);
    if (args[3]) step = atoi(args[3]);
    if (args[4]) samples = atoi(args[4]);
    if (args[5] && strcmp(args[5], "--viz") == 0) {
        terminal_viz = 1;
    }

    // Prepare arrays for visualization
    int n_points = (end_freq - start_freq) / step + 1;
    uint32_t *freq_array = NULL;
    double *power_array = NULL;

    if (terminal_viz) {
        freq_array = malloc(sizeof(uint32_t) * n_points);
        power_array = malloc(sizeof(double) * n_points);
        if (!freq_array || !power_array) {
            perror("Failed to allocate memory for visualization");
            if (freq_array) free(freq_array);
            if (power_array) free(power_array);
            terminal_viz = 0;
        }
    }

    // Open device
    rtlsdr_dev_t *dev;
    if (!open_sdr_device(&dev)) {
        if (terminal_viz) {
            free(freq_array);
            free(power_array);
        }
        return 1;
    }

    // Create output file
    char* timestamp = get_timestamp_string();
    char filename[PATH_MAX];
    sprintf(filename, "%s/spectrum_%s.csv", SPECTRUM_DIR, timestamp);
    free(timestamp);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open output file");
        if (terminal_viz) {
            free(freq_array);
            free(power_array);
        }
        close_sdr_device(dev);
        return 1;
    }

    // Write CSV header
    fprintf(file, "Frequency,Power\n");

    if (!terminal_viz) {
        printf("Scanning from %.2f MHz to %.2f MHz with %.2f kHz steps...\n",
               start_freq/1e6, end_freq/1e6, step/1e3);
    }

    // Allocate buffer for samples
    uint8_t *buffer = malloc(DEFAULT_BUFFER_SIZE);
    if (!buffer) {
        perror("Failed to allocate sample buffer");
        if (terminal_viz) {
            free(freq_array);
            free(power_array);
        }
        fclose(file);
        close_sdr_device(dev);
        return 1;
    }

    // Scan frequencies
    int point_index = 0;
    for (uint32_t freq = start_freq; freq <= end_freq; freq += step) {
        // Set frequency
        rtlsdr_set_center_freq(dev, freq);

        // Reset buffer
        rtlsdr_reset_buffer(dev);

        // Calculate average power
        double power_sum = 0.0;

        for (int i = 0; i < samples; i++) {
            int n_read = 0;
            rtlsdr_read_sync(dev, buffer, DEFAULT_BUFFER_SIZE, &n_read);

            // Calculate power (simple method)
            double power = 0.0;
            for (int j = 0; j < n_read; j++) {
                // Convert to signed value
                double sample = (buffer[j] - 127.5) / 127.5;
                power += sample * sample;
            }

            power /= n_read;
            power_sum += power;
        }

        double avg_power = power_sum / samples;

        // Store data for visualization
        if (terminal_viz && point_index < n_points) {
            freq_array[point_index] = freq;
            power_array[point_index] = avg_power;
            point_index++;

            // Update visualization every few steps
            if (point_index % 5 == 0 || freq >= end_freq) {
                display_terminal_spectrum(freq_array, power_array, point_index, freq);
            }
        } else {
            // Original progress output
            printf("\rScanning %.2f MHz...", freq/1e6);
            fflush(stdout);
        }

        // Write to CSV
        fprintf(file, "%u,%.6f\n", freq, avg_power);
    }

    // Cleanup
    if (terminal_viz) {
        // Show final visualization
        display_terminal_spectrum(freq_array, power_array, point_index, end_freq);

        // Clean up visualization arrays
        free(freq_array);
        free(power_array);

        printf("\nScan complete. Results saved to %s\n", filename);
    } else {
        printf("\nScan complete. Results saved to %s\n", filename);
    }

    free(buffer);
    fclose(file);
    close_sdr_device(dev);

    return 1;
}