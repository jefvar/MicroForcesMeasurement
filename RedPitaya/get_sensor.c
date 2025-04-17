#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "rp.h"

#define PORT 5000
#define S_SENSOR 579.43             // mV/mN
#define THRESHOLD 5.0              // Threshold value for force increment

double OffSet = 2.5;  // Initialize offset value, will be updated with calibration

void config_ADC() {
    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_64);  // 125MS/s => 125/64 = 1.953 MS/s millions of samples per sec
    rp_AcqSetTriggerLevel(RP_CH_1, 0.1);  // Trigger voltage level
    rp_AcqSetTriggerDelay(0);  // No delay
    rp_AcqSetGain(RP_CH_1, RP_LOW);  // Channel 1 ±20 V
}

double calculateForce(double voltage, double offset) {
    return 1000*(voltage - offset) / S_SENSOR;
}

float movingAverageFilter(float *buffer, int n_samples, int block_size) {
    float temp_buffer[n_samples - block_size + 1];
    for (int i = 0; i <= n_samples - block_size; i++) {
        float sum = 0;
        for (int j = 0; j < block_size; j++) {
            sum += buffer[i + j];
        }
        temp_buffer[i] = sum / block_size;
    }

    float avg_force = 0;
    for (int i = 0; i < n_samples - block_size + 1; i++) {
        avg_force += temp_buffer[i];
    }

    return avg_force / (n_samples - block_size + 1);
}

// Function to calculate the offset using moving average filter
double calculateOffset(int n_samples, int sample_block_length) {
    float buffer[n_samples];
    float temp_buffer[n_samples - sample_block_length + 1];
    uint32_t size = n_samples;

    rp_AcqStart();
    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);  // Immediate trigger
    rp_acq_trig_state_t state;
    do {
        rp_AcqGetTriggerState(&state);
    } while (state != RP_TRIG_STATE_TRIGGERED);

    rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);  // Read data from channel 1
    rp_AcqStop();
    // Moving Average Filter
    for (int i = 0; i <= n_samples - sample_block_length; i++) {
        float sum = 0;
        for (int j = 0; j < sample_block_length; j++) {
            sum += buffer[i + j];
        }
        temp_buffer[i] = sum / sample_block_length;
    }

    // Mean value for offset
    double offset_value = 0;
    for (int i = 0; i < n_samples - sample_block_length + 1; i++) {
        offset_value += temp_buffer[i];
    }

    offset_value /= (n_samples - sample_block_length + 1);
    return 10*offset_value;
}

int main() {
    // Initialize API
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Error while initializing API\n");
        return -1;
    }

    // Create socket
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error in bind");
        return -1;
    }

    // Listen
    listen(server_fd, 1);
    printf("Waiting connection to port %d...\n", PORT);
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0) {
        perror("Error in accepting");
        return -1;
    }

    printf("Client connected\n");

    bool started = false;

    // Configure ADC and calculate initial offset
    config_ADC();
    OffSet = calculateOffset(2048, 16);  // Calculate offset using 1024 samples and block size 16
    printf("Calculated Offset: %.5f V\n", OffSet);

    //uint32_t size = 16384;
    uint32_t size = 100;
    float buffer[size];
    rp_acq_trig_state_t state;
    double avg_force;
    int flag = 0;

    while (1) {
        char cmd;
        if (recv(client_fd, &cmd, 1, MSG_DONTWAIT) > 0) {
            if (cmd == 'A') {
                started = true;
            } else if (cmd == 'S') {
                started = false;
            }
        }

        if (started) {
            // Start acquisition
            rp_AcqStart();
            rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);  // Immediate trigger
            do {
                rp_AcqGetTriggerState(&state);
            } while (state != RP_TRIG_STATE_TRIGGERED);
            rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);  // Read data from channel 1

            // Calculate force for each sample
            float forces[size];
            for (int i = 0; i < size; i++) {
                forces[i] = calculateForce(10*buffer[i], OffSet);  // Convert voltage to force
                printf("Vin: %.5f V\t F: %.5f\n", 10*buffer[i], forces[i]);
            }

            // Apply moving average filter to get the average force
            avg_force = movingAverageFilter(forces, size, 16);

            // Check if force exceeds threshold
            if (avg_force > THRESHOLD) {
                flag = 1;
            } else {
                flag = 0;
            }

            // Create a TCP buffer with the force and flag
            char tcp_buffer[1500];
            snprintf(tcp_buffer, sizeof(tcp_buffer), "%.4f,%d\n", avg_force, flag);

            // Send the buffer with force and flag over TCP
            send(client_fd, tcp_buffer, strlen(tcp_buffer), 0);
        }
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}
