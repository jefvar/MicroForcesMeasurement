// LandingRobot.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../../CommROBOT.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <cmath>
#include <winSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

char IP_RedPitaya[] = "169.254.133.97";
u_short PORT_RedPitaya = 4200;

int main()
{
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[1000], server_reply[2000];
    int recv_size;

    // Init socket 
    std::cout << "Starting Winsock..." << std::endl;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "Error: %d" << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "Initialized" << std::endl;
    
    // Create socket 
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        std::cout << "Cannot create the socket: " << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "Socket Created" << std::endl;
    
    // IP Conf
    server.sin_addr.s_addr = inet_addr(IP_RedPitaya);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT_RedPitaya);

    // Connecting
    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cout << "Connecting failure" << std::endl;
        return 1;
    }
    std::cout << "Connected to Red Pitaya" << IP_RedPitaya << "through port" << PORT_RedPitaya << std::endl;

    // Send command to ask for Voltage data
    sprintf_s(message, "A\n");                                   
    send(s, message, strlen(message), 0);
    
    while (1) {
        // Receive data
        if ((recv_size = recv(s, server_reply, strlen(server_reply), 0)) == SOCKET_ERROR) {
            std::cout << "Error while receiving data" << std::endl;
            return 1;
        }
        else {
            server_reply[recv_size] = '\0';
            std::cout << server_reply << std::endl;
        }
    }

    closesocket(s);
    WSACleanup();

    return 0;
}

// Exécuter le programme : Ctrl+F5 ou menu Déboguer > Exécuter sans débogage
// Déboguer le programme : F5 ou menu Déboguer > Démarrer le débogage

// Astuces pour bien démarrer : 
//   1. Utilisez la fenêtre Explorateur de solutions pour ajouter des fichiers et les gérer.
//   2. Utilisez la fenêtre Team Explorer pour vous connecter au contrôle de code source.
//   3. Utilisez la fenêtre Sortie pour voir la sortie de la génération et d'autres messages.
//   4. Utilisez la fenêtre Liste d'erreurs pour voir les erreurs.
//   5. Accédez à Projet > Ajouter un nouvel élément pour créer des fichiers de code, ou à Projet > Ajouter un élément existant pour ajouter des fichiers de code existants au projet.
//   6. Pour rouvrir ce projet plus tard, accédez à Fichier > Ouvrir > Projet et sélectionnez le fichier .sln.
