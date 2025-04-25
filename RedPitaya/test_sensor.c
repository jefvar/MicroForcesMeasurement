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
    rp_AcqSetDecimation(RP_DEC_256);  // 125MS/s => 125/256 = 0.4883 MS/s millions of samples per sec
    rp_AcqSetTriggerLevel(RP_CH_1, 2);  // Trigger voltage level
    rp_AcqSetTriggerDelay(0);  // No delay
    rp_AcqSetGain(RP_CH_1, RP_LOW);  // Channel 1 ±20 V
}

double calculateForce(double voltage, double offset) {
    return 1000*(voltage - offset) / S_SENSOR;
}

void saveForceDataToCSV(float *buffer, int size, const char *filename, double Ts_us) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("No se pudo abrir el archivo para guardar datos");
        return;
    }

    fprintf(fp, "Time_us,Voltage_V,Force_mN\n");

    for (int i = 0; i < size; i++) {
        double time_us = i * Ts_us;
        double voltage = 10*buffer[i];
        fprintf(fp, "%.3f,%.5f\n", time_us, voltage);
    }

    fclose(fp);
    printf("Data saved in %s\n", filename);
}

double movingAverageFilter(float *buffer, int n_samples, int block_size){
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
double calculateOffset(int n_samples) {
    int sample_block_length = 4;
    float buffer[n_samples];
    uint32_t size = n_samples;

    rp_AcqStart();
    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);  // Immediate trigger
    rp_acq_trig_state_t state;
    do {
        rp_AcqGetTriggerState(&state);
    } while (state != RP_TRIG_STATE_TRIGGERED);

    rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);
    rp_AcqStop();

    saveForceDataToCSV(buffer, size, "/tmp/force_data.csv", 2.048);  // 2.048 µs por muestra con decimación 64

    double offset_value = movingAverageFilter(buffer, n_samples, sample_block_length);
    return 10 * offset_value;
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
    OffSet = calculateOffset(16384);  // Calculate offset using 1024 samples and block size 16
    printf("Calculated Offset: %.5f V\n", OffSet);

    uint32_t size = 1024;
    //uint32_t size = 100;
    float buffer[size];
    rp_acq_trig_state_t state;
    //double avg_force;
    //int flag = 0;

    usleep(1000);

    FILE *fp_clear = fopen("/tmp/data_voltage.csv", "w");
    if (fp_clear == NULL) {
        perror("No se pudo limpiar el archivo");
        return -1;
    }
    fclose(fp_clear);

    FILE *fp = fopen("/tmp/data_voltage.csv", "a");  // abre una vez antes del bucle
    if (fp == NULL) {
        perror("No se pudo abrir el archivo para escritura");
        return -1;
    }

    double total_time = 0.0;

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
            //rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);  // Trigger por nivel de voltaje: Ver config ADC
            do {
                rp_AcqGetTriggerState(&state);
            } while (state != RP_TRIG_STATE_TRIGGERED);
            rp_AcqGetOldestDataV(RP_CH_1, &size, buffer);  // Read data from channel 1

            // Voltage values
            double Ts = 256.0 / 125;                // (256.0 / 125e6)*1e6 This value is microseconds
            for (int j = 0; j < 1000; j++) {
                for (int i = 0; i < size; i++) {
                    total_time += Ts;       // tiempo en u_segundos
                    double voltage = 10*buffer[i];          // convertir a voltaje real
                    //printf("Time: %.4f, Voltage: %.6f\n", total_time, voltage);
                    fprintf(fp, "%.4f,%.6f\n", total_time, voltage);
                }
                fflush(fp);
                if (j == 999) {
                    fclose(fp);
                    started = false;
                }
            }
            
            
            //usleep(100000);
        }
            
    }
    

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}
