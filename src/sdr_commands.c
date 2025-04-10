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

// Command to display RTL-SDR device information
int cmd_sdr_info(char **args) {
    int device_count = rtlsdr_get_device_count();

    if (device_count == 0) {
        printf("No RTL-SDR devices found\n");
        return 1;
    }

    printf("Found %d RTL-SDR device(s):\n", device_count);

    for (int i = 0; i < device_count; i++) {
        char vendor[256], product[256], serial[256];

        rtlsdr_get_device_usb_strings(i, vendor, product, serial);

        printf("Device %d:\n", i);
        printf("  Vendor:  %s\n", vendor);
        printf("  Product: %s\n", product);
        printf("  Serial:  %s\n", serial);
        printf("  Name:    %s\n", rtlsdr_get_device_name(i));

        // Try to open the device to get more info
        rtlsdr_dev_t *dev;
        if (rtlsdr_open(&dev, i) == 0) {
            uint32_t rate = rtlsdr_get_sample_rate(dev);
            uint32_t freq = rtlsdr_get_center_freq(dev);
            int gains_count = rtlsdr_get_tuner_gains(dev, NULL);

            printf("  Sample Rate: %u Hz\n", rate);
            printf("  Frequency:   %u Hz\n", freq);
            printf("  Gain Modes:  %d\n", gains_count);

            // Get available gain values
            if (gains_count > 0) {
                int *gains = malloc(sizeof(int) * gains_count);
                rtlsdr_get_tuner_gains(dev, gains);

                printf("  Gain Values: ");
                for (int j = 0; j < gains_count; j++) {
                    printf("%.1f dB", gains[j] / 10.0);
                    if (j < gains_count - 1) {
                        printf(", ");
                    }
                }
                printf("\n");

                free(gains);
            }

            rtlsdr_close(dev);
        }
    }

    return 1;
}

// Command to scan frequency range and log spectrum data
int cmd_sdr_scan(char **args) {
    if (!create_data_directories()) {
        return 1;
    }

    uint32_t start_freq = DEFAULT_FREQ;
    uint32_t end_freq = DEFAULT_FREQ + 10000000; // Default: 10 MHz range
    uint32_t step = 100000; // 100 kHz steps
    int samples = 10; // Number of samples per frequency

    // Parse command arguments
    if (args[1]) start_freq = atoi(args[1]);
    if (args[2]) end_freq = atoi(args[2]);
    if (args[3]) step = atoi(args[3]);
    if (args[4]) samples = atoi(args[4]);

    // Open device
    rtlsdr_dev_t *dev;
    if (!open_sdr_device(&dev)) {
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
        close_sdr_device(dev);
        return 1;
    }

    // Write CSV header
    fprintf(file, "Frequency,Power\n");

    printf("Scanning from %.2f MHz to %.2f MHz with %.2f kHz steps...\n",
           start_freq/1e6, end_freq/1e6, step/1e3);

    // Allocate buffer for samples
    uint8_t *buffer = malloc(DEFAULT_BUFFER_SIZE);
    if (!buffer) {
        perror("Failed to allocate sample buffer");
        fclose(file);
        close_sdr_device(dev);
        return 1;
    }

    // Scan frequencies
    for (uint32_t freq = start_freq; freq <= end_freq; freq += step) {
        printf("\rScanning %.2f MHz...", freq/1e6);
        fflush(stdout);

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

        // Write to CSV
        fprintf(file, "%u,%.6f\n", freq, avg_power);
    }

    printf("\nScan complete. Results saved to %s\n", filename);

    free(buffer);
    fclose(file);
    close_sdr_device(dev);

    return 1;
}

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
//    char* timestamp = get_timestamp_string();
//    char filename[PATH_MAX];
//    sprintf(filename, "%s/iq_%s.dat", IQ_DIR, timestamp);
    // After generating the timestamp
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

// Command to monitor a specific frequency and measure SNR
int cmd_sdr_snr(char **args) {
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
    sprintf(filename, "%s/snr_%s.csv", SNR_DIR, timestamp);
    free(timestamp);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open output file");
        close_sdr_device(dev);
        return 1;
    }

    // Write CSV header
    fprintf(file, "Time,SignalPower,NoisePower,SNR\n");

    // Reset buffer
    rtlsdr_reset_buffer(dev);

    printf("Measuring SNR at %.2f MHz for %u seconds...\n", freq/1e6, duration);

    // Calculate number of samples based on duration and sample rate
    uint32_t buffer_size = DEFAULT_BUFFER_SIZE;
    uint8_t *buffer = malloc(buffer_size);

    if (!buffer) {
        perror("Failed to allocate sample buffer");
        fclose(file);
        close_sdr_device(dev);
        return 1;
    }

    // Calculate FFT size (must be power of 2)
    int fft_size = 1024;

    // Allocate FFT buffers
    fftwf_complex *fft_in = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_complex *fft_out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * fft_size);
    fftwf_plan fft_plan = fftwf_plan_dft_1d(fft_size, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

    if (!fft_in || !fft_out || !fft_plan) {
        fprintf(stderr, "Failed to allocate FFT resources\n");
        if (fft_in) fftwf_free(fft_in);
        if (fft_out) fftwf_free(fft_out);
        free(buffer);
        fclose(file);
        close_sdr_device(dev);
        return 1;
    }

    // Record and process data
    time_t start_time = time(NULL);
    time_t current_time = start_time;

    while (current_time - start_time < duration) {
        int n_read = 0;
        rtlsdr_read_sync(dev, buffer, buffer_size, &n_read);

        if (n_read > 0) {
            // Process buffer in FFT-sized chunks
            for (int offset = 0; offset + (fft_size * 2) <= n_read; offset += (fft_size * 2)) {
                // Convert samples to complex format for FFT
                for (int i = 0; i < fft_size; i++) {
                    // Convert 8-bit unsigned to float
                    float I = (buffer[offset + (i * 2)] - 127.5f) / 127.5f;
                    float Q = (buffer[offset + (i * 2) + 1] - 127.5f) / 127.5f;

                    fft_in[i][0] = I;
                    fft_in[i][1] = Q;
                }

                // Perform FFT
                fftwf_execute(fft_plan);

                // Calculate power spectrum
                float power_spectrum[fft_size];
                for (int i = 0; i < fft_size; i++) {
                    float re = fft_out[i][0];
                    float im = fft_out[i][1];
                    power_spectrum[i] = re*re + im*im;
                }

                // Center bin (DC)
                int center_bin = 0;

                // Signal power (center and adjacent bins)
                float signal_power = 0.0f;
                for (int i = center_bin - 2; i <= center_bin + 2; i++) {
                    int idx = (i + fft_size) % fft_size; // Wrap around
                    signal_power += power_spectrum[idx];
                }
                signal_power /= 5.0f;

                // Noise power (all other bins)
                float noise_power = 0.0f;
                int noise_bins = 0;
                for (int i = 0; i < fft_size; i++) {
                    if (i < center_bin - 2 || i > center_bin + 2) {
                        noise_power += power_spectrum[i];
                        noise_bins++;
                    }
                }
                noise_power /= noise_bins;

                // Calculate SNR
                float snr = signal_power / noise_power;
                float snr_db = 10.0f * log10f(snr);

                // Write to CSV
                fprintf(file, "%ld,%.6f,%.6f,%.2f\n",
                        time(NULL) - start_time, signal_power, noise_power, snr_db);

                // Print update
                printf("\rSNR: %.2f dB", snr_db);
                fflush(stdout);
            }
        }

        current_time = time(NULL);
    }

    printf("\nSNR measurement complete. Results saved to %s\n", filename);

    // Clean up
    fftwf_destroy_plan(fft_plan);
    fftwf_free(fft_in);
    fftwf_free(fft_out);
    free(buffer);
    fclose(file);
    close_sdr_device(dev);

    return 1;
}

// Command to monitor a frequency (simplified version)
int cmd_sdr_monitor(char **args) {
    uint32_t freq = DEFAULT_FREQ;

    // Parse command arguments
    if (args[1]) freq = atoi(args[1]);

    // Open device
    rtlsdr_dev_t *dev;
    if (!open_sdr_device(&dev)) {
        return 1;
    }

    // Set frequency
    rtlsdr_set_center_freq(dev, freq);

    // Reset buffer
    rtlsdr_reset_buffer(dev);

    printf("Monitoring %.2f MHz. Press Ctrl+C to stop...\n", freq/1e6);

    // Allocate buffer
    uint32_t buffer_size = DEFAULT_BUFFER_SIZE;
    uint8_t *buffer = malloc(buffer_size);

    if (!buffer) {
        perror("Failed to allocate sample buffer");
        close_sdr_device(dev);
        return 1;
    }

    // Simple monitoring loop
    while (1) {
        int n_read = 0;
        rtlsdr_read_sync(dev, buffer, buffer_size, &n_read);

        if (n_read > 0) {
            // Calculate simple power level
            double power = 0.0;
            for (int i = 0; i < n_read; i++) {
                double sample = (buffer[i] - 127.5) / 127.5;
                power += sample * sample;
            }
            power /= n_read;

            // Display power meter
            int meter_width = 50;
            int bars = (int)(power * meter_width * 10.0);
            if (bars > meter_width) bars = meter_width;

            printf("\rSignal: [");
            for (int i = 0; i < meter_width; i++) {
                printf("%c", i < bars ? '#' : ' ');
            }
            printf("] %.2f dB", 10.0 * log10(power));
            fflush(stdout);
        }

        // Check for Ctrl+C (would need signal handling for real implementation)
        // For this demo, we just do a small number of iterations
        static int count = 0;
        if (++count >= 100) break;

        usleep(100000); // 100ms
    }

    printf("\nMonitoring stopped.\n");

    free(buffer);
    close_sdr_device(dev);

    return 1;
}