#ifndef COMMROBOT_H
#define COMMROBOT_H

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <string>
#include <math.h>
#include <time.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define BUFFER_MSG 100
#define BUFFER_SMS 100

class CommROBOT
{
public:
    CommROBOT();
    ~CommROBOT();
    bool Connect(char*, unsigned short);
    bool SetSpdCtrl(bool On);
    bool SetSpdCmd(double vx, double vy, double vz, double wx, double wy, double wz);
    bool SetLawSpeed(double Sp, double Accel, double Decel, double Jerk, bool Arret, double Distance);
    bool SetTCP(double x, double y, double z, double rx, double ry, double rz);
    bool CmdGoPose(double J1, double J2, double J3, double J4, double J5, double J6);
    bool CmdGoGeoPose(double X, double Y, double Z, double RX, double RY, double RZ);
    bool GetJPos(double* J1, double* J2, double* J3, double* J4, double* J5, double* J6);
    bool GetGeoPos(double* X, double* Y, double* Z, double* RX, double* RY, double* RZ);
    bool SetTypeMov(double type_jog);
    bool Homing(double J);
    bool MoveAxe(double njog);
    bool SetSpeed(double percentage);
    bool Stop(double njog);

protected:

private:
    WSADATA WSAData; // configuration socket
    SOCKET sock;
    SOCKADDR_IN sin;
    bool Connecte;
    char bufferMsg[BUFFER_MSG];
};

#endif // COMMROBOT_H