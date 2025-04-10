#include "shell.h"

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