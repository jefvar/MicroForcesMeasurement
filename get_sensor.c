#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "rp.h"

#define PORT 5000

int main() {
    // Iniciar API
    if (rp_Init() != RP_OK) {
        fprintf(stderr, "Error al iniciar API\n");
        return -1;
    }

    // Crear socket
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
        perror("Error en bind");
        return -1;
    }

    // Escuchar
    listen(server_fd, 1);
    printf("Esperando conexión en puerto %d...\n", PORT);
    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (client_fd < 0) {
        perror("Error en accept");
        return -1;
    }

    printf("Cliente conectado\n");

    // Loop de adquisición y envío
    while (1) {
        float value;
        uint32_t raw_value;  // Variable para almacenar el valor crudo
        rp_AIpinGetValue(RP_AIN0, &value, &raw_value);

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.4f\n", value);

        send(client_fd, buffer, strlen(buffer), 0);

        usleep(10000); // 10 ms -> 100 Hz
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}