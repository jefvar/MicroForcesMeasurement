#include "../../CommROBOT.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <cmath>

CommROBOT robot;

struct Position3D {
    double x, y, z, rx, ry, rz;
    Position3D() : x(0), y(0), z(0), rx(0), ry(0), rz(0) {}              // Init in zero

    void set_pos(double new_x, double new_y, double new_z) {
        x = new_x;
        y = new_y;
        z = new_z;
    }

    void set_rot(double new_rx, double new_ry, double new_rz) {
        rx = new_rx;
        ry = new_ry;
        rz = new_rz;
    }

    void print() {
        std::cout << "Current Position: " << x << ", " << y << ", " << z << ", " << rx << ", " << ry << ", " << rz << std::endl;
    }
};

int main() {


    //IP direction (char[])
    char ip[] = "172.16.100.244";

    // Conectting to robot
    if (!robot.Connect(ip, 2726)) {
        std::cerr << "Error while trying to connect to the robot" << std::endl;
        return 1;
    }
    else {
        std::cout << "Connected successfully" << std::endl;
    }

    /*if (robot.Homing(6)) {
        std::cout << "Home ok" << std::endl;
    }
    else {
        std::cerr << "Error" << std::endl;
    }*/
    //robot.SetTypeMov(11);
    robot.MoveAxe(-1);
    robot.SetSpeed(100000);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    robot.Stop(1);

    return 0;
}