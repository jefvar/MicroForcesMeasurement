#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "rp.h"

#define PORT 4200

int main() {
    // Init API
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
        perror("Socket falló");
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

    while (1) {
        char cmd;
        float value;
        uint32_t raw_value;

        if (recv(client_fd, &cmd, 1, MSG_DONTWAIT) > 0 ) 
        {
            if(cmd == 'A') {
                started = true;
            } else if (cmd == 'S') {
                started = false;
            }
        }
        
        
        if (started) {
            rp_AIpinGetValue(RP_AIN0, &value, &raw_value);              // Red ADC in AIN0

            char buffer[1500];
            snprintf(buffer, sizeof(buffer), "%.4f\n", value);

            // Show in console
            //printf("Valor leído: %.4f V\n", value);

            // Send to socket
            send(client_fd, buffer, strlen(buffer), 0);

        }

        usleep(10000); // 10 ms -> 100 Hz
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}