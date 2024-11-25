#include <iostream>
#include "daly-bms-uart.h"
#include <time.h>
#include <chrono> // Include chrono for timing
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./libraries/cpp-httplib/httplib.h"


int main2(int argc, char **argv) {
    httplib::SSLClient client("catfact.ninja");
    client.enable_server_certificate_verification(true);
    auto result = client.Get("/fact");
    if (result) {
        std::cout << result->status << std::endl;
        std::cout << result->body << std::endl;
    } else {
        auto err = result.error();
        std::cout << "Error: " << err << std::endl;
    }
    return 0;
}


int main(int argc, char **argv) {
    Daly_BMS_UART bms(argv[1]);
    bms.Init();
    
    while ( true ) {
        auto start = std::chrono::high_resolution_clock::now(); // Start timing

        bms.update();

        std::cout << "\033[0mbasic BMS Data:             \033[32;1m" << bms.get.packVoltage << " V," << bms.get.packCurrent << " A " << bms.get.packSOC << "\% " << std::endl;
        std::cout << "\033[0mPackage Temperature:        \033[32;1m" << bms.get.tempAverage << std::endl;
        std::cout << "\033[0mHighest Cell Voltage Nr:    \033[32;1m" << bms.get.maxCellVNum << " with voltage" << (bms.get.maxCellmV / 1000) << std::endl;
        std::cout << "\033[0mLowest Cell Voltage Nr:     \033[32;1m" << bms.get.minCellVNum << " with voltage" << (bms.get.minCellmV / 1000) << std::endl;
        std::cout << "\033[0mNumber of Cells:            \033[32;1m" << bms.get.numberOfCells << std::endl;
        std::cout << "\033[0mNumber of Temp Sensors:     \033[32;1m" << bms.get.numOfTempSensors << std::endl;
        std::cout << "\033[0mBMS Chrg / Dischrg Cycles:  \033[32;1m" << bms.get.bmsCycles << std::endl;
        std::cout << "\033[0mBMS Heartbeat:              \033[32;1m" << bms.get.bmsHeartBeat << std::endl; // cycle 0-255
        std::cout << "\033[0mDischarge MOSFet Status:    \033[32;1m" << bms.get.disChargeFetState << std::endl;
        std::cout << "\033[0mCharge MOSFet Status:       \033[32;1m" << bms.get.chargeFetState << std::endl;
        std::cout << "\033[0mRemaining Capacity mAh:     \033[32;1m" << bms.get.resCapacitymAh << std::endl;

        auto end = std::chrono::high_resolution_clock::now(); // End timing
        std::chrono::duration<double, std::milli> elapsed = end - start; // Calculate elapsed time
        std::cout << "\033[0mLoop execution time:        \033[32;1m" << elapsed.count() << " ms" << std::endl;

        std::cout << "\033[0m\033[12A\033[0G";
    }
    return 0;
}
