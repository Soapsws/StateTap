#include "statetap/statetap.hpp"

#include <iostream>

int main() {
    statetap::initialize();
    double speed = 1.5;
    statetap::tap("robot/speed", speed);
    statetap::shutdown();
    std::cout << "StateTap example compiled; telemetry is not implemented yet.\n";
}

