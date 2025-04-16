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

Position3D InitialPos;                                // Initial position (coordinates frame)
//double x0, y0, z0, rx0, ry0, rz0;                   
//double x, y, z, rx, ry, rz;                         
Position3D ActualPos;                                 // Actual position (coordinates frame)
Position3D RefPos;                                    // Desired end reference position (fixed, coordinates frame)
double Vz_ref = -1;
double Vx = 0;
double Vy = 0;
double Vz = 0;
//double Z_ref = 0;
double t = 0;
//double deltaT = 0;
//double Zf = 0;
Position3D TrajectoryPos;                             // Reference trajectory (coordinates frame)
double k;
double error = 0;

std::atomic<bool> running(true);

void example_trajectory(char type, double tf, double t, Position3D InitialPos, Position3D RefPos);
void landing(double tf, double t, Position3D RefPos);

void ISR_int(int ts) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ts));
        t = t + ts;
        std::cout << "Tiempo =" << t << std::endl;

        // Read the Z position
        
        if (robot.GetGeoPos(&ActualPos.x, &ActualPos.y, &ActualPos.z, &ActualPos.rx, &ActualPos.ry, &ActualPos.rz)) {
            ActualPos.print();
        }
        else {
            std::cerr << "Error while reading the position" << std::endl;
        }

        // Set desired position
        //Z_ref = -10;
        RefPos.set_pos(-41.6, 0, -20);
        RefPos.set_rot(-180, 90, 0);
        
        //deltaT = 10;

        double tf = 5;
        
        k = 10 / (tf);

        /*TrajectoryPos.x = InitialPos.x + (RefPos.x - InitialPos.x) * (1 / (1 + exp(-k * (t - tf))));
        TrajectoryPos.y = InitialPos.y + (RefPos.y - InitialPos.y) * (1 / (1 + exp(-k * (t - tf))));
        TrajectoryPos.z = InitialPos.z + (RefPos.z - InitialPos.z) * (1 / (1 + exp(-k * (t - tf))));

        std::cout << "Posicion deseada X Y Z: =" << TrajectoryPos.x << ", " << TrajectoryPos.y << ", " << TrajectoryPos.z << std::endl;

        robot.CmdGoGeoPose(TrajectoryPos.x, TrajectoryPos.y, TrajectoryPos.z, RefPos.rx, RefPos.ry, RefPos.rz);*/

        //example_trajectory('A', 20000, t, InitialPos, RefPos);
        landing(5000, t, RefPos);

    }
}

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
    
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    int ts = 100;                   // Define sample time in ms

    // Get initial position
    if (robot.GetGeoPos(&InitialPos.x, &InitialPos.y, &InitialPos.z, &InitialPos.rx, &InitialPos.ry, &InitialPos.rz)) {
        std::cout << "Initial Position:" << std::endl;
        InitialPos.print();
    }
    else {
        std::cerr << "Error while reading the position" << std::endl;
    }
    
    std::thread sampling(ISR_int, ts);                  // Trajectory planning thought sampling
    
    //robot.CmdGoPose(10, 10, -10, 0, 0, 0);
    //robot.SetTypeMov(10);
    //robot.CmdGoPose(10, 00, 0, 0, 0, 0);
    //robot.CmdGoGeoPose(-41.56, 0, 0, 180, 90, 0);
    //robot.Homing();        
    
    while (running) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 27) {
                running = false;
            }
        }
        /*if (t > 10000) {
            running = false;
        }*/
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    sampling.join();
    std::cout << "Program stopped:" << std::endl;
    
    return 0;
}

void example_trajectory(char type, double tf, double t, Position3D InitialPos, Position3D RefPos) {
    double k=0;
    if (type == 'A') {
        // Square trajectory
        /*
        *   InitialPos----------
        *   |                   |
        *   |                   |
        *   ---------------RefPos
        */
        double a = abs(RefPos.x - InitialPos.x);
        double b = abs(RefPos.y - InitialPos.y);
        double l =  2*a + 2*b;
        double t1 = (a / l) * tf;
        double t2 = (b / l) * tf;
        
        if (t < t1) {
            k = 10 / (t1);
            TrajectoryPos.x = InitialPos.x + (RefPos.x - InitialPos.x) * (1 / (1 + exp(-k * (t - (0 + t1)/2))));
        }
        else if (t >= t1 && t < (t1+t2)) {
            k = 10 / (t2);
            TrajectoryPos.y = InitialPos.y + (RefPos.y - InitialPos.y) * (1 / (1 + exp(-k * (t - (2*t1 + t2)/2))));
        }
        else if (t >= (t1 + t2) && t < (2 * t1 + t2)) {
            k = 10 / (t1);
            TrajectoryPos.x = RefPos.x + (InitialPos.x - RefPos.x) * (1 / (1 + exp(-k * (t - (2*t2 + 3*t1)/2))));
        }
        else if (t >= (2 * t1 + t2) && t < tf) {
            k = 10 / (t2);
            TrajectoryPos.y = RefPos.y + (InitialPos.y - RefPos.y) * (1 / (1 + exp(-k * (t - (t2 + 2 * t1 + tf)/2))));
        }
        else {
            running = false;

        }
        TrajectoryPos.z = InitialPos.z + (RefPos.z - InitialPos.z) * (1 / (1 + exp(-(10/tf) * (t - tf/2))));
        std::cout << "Posicion deseada X Y Z: =" << TrajectoryPos.x << ", " << TrajectoryPos.y << ", " << TrajectoryPos.z << std::endl;
        robot.CmdGoGeoPose(TrajectoryPos.x, TrajectoryPos.y, TrajectoryPos.z, RefPos.rx, RefPos.ry, RefPos.rz);

    }
}

void landing(double tf, double t, Position3D RefPos) {
    /*********************** Smooth trajectory only in Z direction ***********************/
    double k = 10 / tf;
    
    // Get Actual Position
    /*if (robot.GetGeoPos(&InitialPos.x, &InitialPos.y, &InitialPos.z, &InitialPos.rx, &InitialPos.ry, &InitialPos.rz)) {
        std::cout << "Initial Position:" << std::endl;
        InitialPos.print();
    }
    else {
        std::cerr << "Error while reading the position" << std::endl;
    }*/
    // Speed control: TRUE
    robot.SetSpdCtrl(true);
    // Go to Final Position
    if (t <= tf) {
        TrajectoryPos.x = InitialPos.x + (RefPos.x - InitialPos.x) * (t - 0) / (tf);
        TrajectoryPos.y = InitialPos.y + (RefPos.y - InitialPos.y) * (t - 0) / (tf);
        TrajectoryPos.z = InitialPos.z + (RefPos.z - InitialPos.z) * (1 / (1 + exp(-k * (t - tf / 2))));
        Vx = (1000 / 5)*(RefPos.x - InitialPos.x) / (tf);
        Vy = (1000 / 5)*(RefPos.x - InitialPos.x) / (tf);
        Vz = (1000 / 5)*(RefPos.z - InitialPos.z) * (( k*exp(-k * (t - tf / 2))) / (1 + exp(-k * (t - tf / 2))));
        std::cout << "Velocidades X Y Z: =" << Vx << ", " << Vy << ", " << Vz << std::endl;
        
    }
    else {
        running = false;
        robot.SetSpdCtrl(false);
    }
    std::cout << "Posicion deseada X Y Z: =" << TrajectoryPos.x << ", " << TrajectoryPos.y << ", " << TrajectoryPos.z << std::endl;
    robot.SetSpdCmd(Vx, Vy, Vz, 0.0, 0.0, 0.0);
    //robot.CmdGoGeoPose(TrajectoryPos.x, TrajectoryPos.y, TrajectoryPos.z, RefPos.rx, RefPos.ry, RefPos.rz);
}

void testing() {

}