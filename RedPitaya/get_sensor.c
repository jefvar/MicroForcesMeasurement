#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "rp.h"

#define PORT 5000
#define S_SENSOR 579.43             // mV/mN (sensor sensitivity)

double OffSet = 2.5;

// Moving Average Filter
void moving_average_filter(float* input, float* output, int n_samples, int window_size) {
    for (int i = 0; i <= n_samples - window_size; i++) {
        float sum = 0;
        for (int j = 0; j < window_size; j++) {
            sum += input[i + j];
        }
        output[i] = sum / window_size;
    }
}

void config_ADC() {
    rp_AcqReset();
    rp_AcqSetDecimation(RP_DEC_64);     // 125MS/s => 125/64 = 1.953 MS/s (samples per second)
    rp_AcqSetTriggerLevel(RP_CH_1, 0.1);         // Trigger voltage level
    rp_AcqSetTriggerDelay(0);           // No delay
    rp_AcqSetGain(RP_CH_1, RP_LOW);    // Channel 1 ±20 V
}

// Get offset using moving average filter
float Get_Offset_with_filter(int n_samples, int sample_block_length) {
    float buffer[n_samples];
    float filtered_buffer[n_samples - sample_block_length + 1];
    uint32_t size = n_samples;

    rp_AcqStart();

    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);       // Immediate trigger
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
    do {
        rp_AcqGetTriggerState(&state);
    } while (state != RP_TRIG_STATE_TRIGGERED);

    rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);

    // Apply moving average filter to the voltage data
    moving_average_filter(buffer, filtered_buffer, n_samples, sample_block_length);

    // Calculate the average of the filtered data to get the offset
    float final_acc = 0;
    for (int i = 0; i < n_samples - sample_block_length + 1; i++) {
        final_acc += filtered_buffer[i];
    }

    double offset_value = final_acc / (n_samples - sample_block_length + 1);
    printf("Offset (sample block length: %d): %.5f V\n", sample_block_length, offset_value);
    return offset_value;
}

// Calculate force based on voltage and offset
double GetForce(double Voltage, double Offset) {
    double Force = (Voltage - Offset) / S_SENSOR;
    return Force;
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
    printf("Waiting for connection on port %d...\n", PORT);
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0) {
        perror("Error in accepting");
        return -1;
    }

    printf("Client connected\n");

    bool started = false;

    config_ADC();
    OffSet = Get_Offset_with_filter(1024, 16);  // Use 1024 samples with a block length of 16 for the moving average

    while (1) {
        char cmd;

        // Receive command from the client
        if (recv(client_fd, &cmd, 1, MSG_DONTWAIT) > 0 ) {
            if(cmd == 'A') {
                started = true;
            } else if (cmd == 'S') {
                started = false;
            }
        }

        if (started) {
            // RF Acquisition
            rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);       // Immediate trigger
            rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
            do {
                rp_AcqGetTriggerState(&state);
            } while (state != RP_TRIG_STATE_TRIGGERED);

            // Read data
            uint32_t size = 16384; // Maximum number of samples (depends on decimation)
            float buffer[size];

            rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);  // Read RF IN1

            // Apply moving average filter to the acquired data
            float filtered_buffer[size - 16 + 1];  // Adjust size for the filter
            moving_average_filter(buffer, filtered_buffer, size, 16);

            // Calculate force for each sample
            float forces[size];
            for (int i = 0; i < size - 16 + 1; i++) {
                forces[i] = GetForce(filtered_buffer[i], OffSet);  // Get force from filtered voltage data

                // Print force value (optional)
                printf("Force %d: %.4f N\n", i, forces[i]);

                // If you want to send forces as strings over the socket, convert them to text
                char send_buffer[100];
                snprintf(send_buffer, sizeof(send_buffer), "%.4f\n", forces[i]);
                send(client_fd, send_buffer, strlen(send_buffer), 0);
            }
        }

        //usleep(10000); // 10 ms -> 100 Hz
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}
