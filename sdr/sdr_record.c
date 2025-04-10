#include "shell.h"

// Command to record IQ data samples
int cmd_sdr_record(char **args) {
    if (!create_data_directories()) {
        return 1;
    }

    uint32_t freq = DEFAULT_FREQ;
    uint32_t duration = 10; // Default 10 seconds

    // Parse command arguments
    if (args[1]) freq = atoi(args[1]);
    if (args[2]) duration = atoi(args[2]);

    // Open device
    rtlsdr_dev_t *dev;
    if (!open_sdr_device(&dev)) {
        return 1;
    }

    // Set frequency
    rtlsdr_set_center_freq(dev, freq);

    // Create output file
    char* timestamp = get_timestamp_string();
    char filename[PATH_MAX];

    // Sanitize the timestamp to ensure it only contains safe characters
    for (int i = 0; timestamp[i] != '\0'; i++) {
        // Replace any non-alphanumeric characters with underscores
        if (!isalnum(timestamp[i]) && timestamp[i] != '-' && timestamp[i] != '_') {
            timestamp[i] = '_';
        }
    }

    sprintf(filename, "%s/iq_%s.dat", IQ_DIR, timestamp);
    free(timestamp);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open output file");
        close_sdr_device(dev);
        return 1;
    }

    // Reset buffer
    rtlsdr_reset_buffer(dev);

    printf("Recording IQ data at %.2f MHz for %u seconds...\n", freq/1e6, duration);

    // Calculate number of samples based on duration and sample rate
    uint32_t total_samples = duration * DEFAULT_SAMPLE_RATE;
    uint32_t buffer_size = DEFAULT_BUFFER_SIZE;
    uint8_t *buffer = malloc(buffer_size);

    if (!buffer) {
        perror("Failed to allocate sample buffer");
        fclose(file);
        close_sdr_device(dev);
        return 1;
    }

    // Record data
    uint32_t samples_collected = 0;
    time_t start_time = time(NULL);

    while (samples_collected < total_samples) {
        int n_read = 0;
        rtlsdr_read_sync(dev, buffer, buffer_size, &n_read);

        if (n_read > 0) {
            fwrite(buffer, 1, n_read, file);
            samples_collected += n_read / 2; // Two bytes per sample (I & Q)

            // Update progress
            double progress = (double)samples_collected / total_samples * 100.0;
            printf("\rProgress: %.1f%%", progress);
            fflush(stdout);
        }

        // Check for timeout
        if (time(NULL) - start_time > duration + 5) {
            printf("\nRecording timed out\n");
            break;
        }
    }

    printf("\nRecording complete. IQ data saved to %s\n", filename);

    // Save info file with metadata
    char info_filename[PATH_MAX];
    sprintf(info_filename, "%s/iq_%s.txt", IQ_DIR, timestamp);

    FILE *info_file = fopen(info_filename, "w");
    if (info_file) {
        fprintf(info_file, "Sample Rate: %u Hz\n", DEFAULT_SAMPLE_RATE);
        fprintf(info_file, "Center Frequency: %u Hz\n", freq);
        fprintf(info_file, "Duration: %u seconds\n", duration);
        fprintf(info_file, "Sample Format: 8-bit unsigned IQ\n");
        fclose(info_file);
    }

    free(buffer);
    fclose(file);
    close_sdr_device(dev);

    return 1;
}