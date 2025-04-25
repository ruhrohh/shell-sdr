/**
 * @file sdr_info.c
 * @brief RTL-SDR device information
 *
 * Implements the sdr_info command which displays detailed information
 * about connected RTL-SDR devices, including vendor, product, serial,
 * sample rate, frequency, and available gain values.
 */

#include "shell.h"

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