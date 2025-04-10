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

    // Configurar el modo de adquisición para RF
    rp_AnalogInCalibrate(RP_CH_1);  // Calibración de la entrada RFA
    rp_AnalogInCalibrate(RP_CH_2);  // Calibración de la entrada RFB

    // Bucle de adquisición y envío
    while (1) {
        float valueRFA, valueRFB;
        uint32_t raw_valueRFA, raw_valueRFB;

        // Leer señales de RF de RFA y RFB
        rp_AnalogInGetVolt(RP_CH_1, &valueRFA);  // Leer la entrada RFA
        rp_AnalogInGetVolt(RP_CH_2, &valueRFB);  // Leer la entrada RFB

        // Formato para enviar las señales como texto
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "RFA: %.4f V, RFB: %.4f V\n", valueRFA, valueRFB);

        send(client_fd, buffer, strlen(buffer), 0);

        usleep(10000); // 10 ms -> 100 Hz
    }

    // Cleanup
    close(client_fd);
    close(server_fd);
    rp_Release();

    return 0;
}
