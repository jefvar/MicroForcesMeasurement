#include "CommROBOT.h"
#include <iostream>
#include <ws2tcpip.h>  // Necesario para InetPtonA

CommROBOT::CommROBOT() : Connecte(false), sock(INVALID_SOCKET) {
    memset(&WSAData, 0, sizeof(WSAData));
    memset(&sin, 0, sizeof(sin));
    memset(bufferMsg, 0, BUFFER_MSG);
}

CommROBOT::~CommROBOT() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    WSACleanup();
}

bool CommROBOT::Connect(char* Adresse, unsigned short port) {
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        std::cerr << "Error al inicializar Winsock" << std::endl;
        return false;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (InetPtonA(AF_INET, Adresse, &sin.sin_addr) != 1) {
        std::cerr << "Dirección IP inválida" << std::endl;
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error al crear el socket" << std::endl;
        return false;
    }

    Connecte = (connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR);
    return Connecte;
}

bool CommROBOT::SetSpdCtrl(bool On) {
    char buffer[200];
    int n;
    float val = 0;
    n = sprintf_s(buffer, sizeof(buffer), "SETTORSORVALUE,%f,%f,%f,%f,%f,%f\r\n", val, val, val, val, val, val);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);

    if (On)
        send(sock, "SPEEDCTRLON\r\n", 13, 0);
    else
        send(sock, "SPEEDCTRLOFF\r\n", 14, 0);

    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::SetSpdCmd(double vx, double vy, double vz, double wx, double wy, double wz) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "SETTORSORVALUE,%f,%f,%f,%f,%f,%f\r\n", vx, vy, vz, wx, wy, wz);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::SetLawSpeed(double Sp, double Accel, double Decel, double Jerk, bool Arret, double Distance) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "SETLAWSPEED,%f,%f,%f,%f,%d,%f\r\n", Sp, Accel, Decel, Jerk, Arret, Distance);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::SetTCP(double x, double y, double z, double rx, double ry, double rz) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "SETTOOL,%f,%f,%f,%f,%f,%f\r\n", x, y, z, rx, ry, rz);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::CmdGoPose(double J1, double J2, double J3, double J4, double J5, double J6) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "ADDPOS,%f,%f,%f,%f,%f,%f\r\n", J1, J2, J3, J4, J5, J6);      // Add a position to the move set without smoothing. Related to X, Y, Z, Rx, Ry, Rz
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::CmdGoGeoPose(double X, double Y, double Z, double RX, double RY, double RZ) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "ADDGEOPOS,%f,%f,%f,%f,%f,%f\r\n", X, Y, Z, RX, RY, RZ);      // Add a position to the move set. If available, this point can be smoothed. Related to J1, J2, J3, J4, J5, J6
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::GetJPos(double* J1, double* J2, double* J3, double* J4, double* J5, double* J6) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "GETPOS\r\n");
    send(sock, buffer, n, 0);

    char c = 0;
    n = 0;
    memset(bufferMsg, 0, BUFFER_MSG);

    while (recv(sock, &c, 1, 0) > 0 && c != '\r') {
        if (n < BUFFER_MSG - 1) { // Check to prevent buffer overflow
            bufferMsg[n++] = c;
        }
        else {
            // Handle buffer overflow error
            std::cerr << "Buffer overflow detected" << std::endl;
            return false;
        }
    }

    bufferMsg[n] = '\0'; // Null-terminate the bufferMsg

    return sscanf_s(bufferMsg, "%lf,%lf,%lf,%lf,%lf,%lf", J1, J2, J3, J4, J5, J6) == 6;
}

bool CommROBOT::GetGeoPos(double* X, double* Y, double* Z, double* RX, double* RY, double* RZ) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "GETGEOPOS\r\n");
    send(sock, buffer, n, 0);

    char c = 0;
    n = 0;
    memset(bufferMsg, 0, BUFFER_MSG);

    while (recv(sock, &c, 1, 0) > 0 && c != '\r') {
        if (n < BUFFER_MSG - 1) { // Check to prevent buffer overflow
            bufferMsg[n++] = c;
        }
        else {
            // Handle buffer overflow error
            std::cerr << "Buffer overflow detected" << std::endl;
            return false;
        }
    }

    bufferMsg[n] = '\0'; // Null-terminate the bufferMsg

    return sscanf_s(bufferMsg, "%lf,%lf,%lf,%lf,%lf,%lf", X, Y, Z, RX, RY, RZ) == 6;
}

// Pending testing
bool CommROBOT::SetTypeMov(double type_jog) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "TYPEJOG,%f\r\n", type_jog);      // Chose the type of movement: 10, 11, 12 (articulated, tool frame, actual frame)
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::Homing(double J) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "HOMING, %f\r\n", J);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::MoveAxe(double njog) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "JOGON, %f\r\n", njog);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::SetSpeed(double percentage) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "SETSPEED, %f\r\n", percentage);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

bool CommROBOT::Stop(double njog) {
    char buffer[200];
    int n;
    n = sprintf_s(buffer, sizeof(buffer), "JOGOFF, %f\r\n", njog);
    send(sock, buffer, n, 0);
    recv(sock, bufferMsg, 4, 0);
    return (strcmp(bufferMsg, "OK\r\n") == 0);
}

//bool CommROBOT::GetError() {
//    char buffer[200];
//    int n;
//    n = sprintf_s(buffer, sizeof(buffer), "JOGON, %f\r\n");
//    send(sock, buffer, n, 0);
//    recv(sock, bufferMsg, 4, 0);
//    return (strcmp(bufferMsg, "OK\r\n") == 0);
//}