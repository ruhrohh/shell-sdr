/**
 * @file sdr_snr.c
 * @brief Signal-to-Noise Ratio measurement functionality
 *
 * Implements the sdr_snr command which measures and logs the signal-to-noise
 * ratio at a specific frequency. Performs FFT analysis to separate signal
 * from noise and calculates SNR in dB. Results are saved to CSV files.
 */

#include "shell.h"

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