#include <iostream>
#include "daly-bms-uart.h"
#include <time.h>
#include <chrono> // Include chrono for timing
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./libraries/cpp-httplib/httplib.h"
#include "./libraries/line-protocol/LineProtocol.hpp"

bool sendRequest(const char* url, int port, const char* data, InfluxDBContext dbContext) {
    httplib::Client client(url, port);
    std::string token = "Token " + std::string(dbContext.token);
    httplib::Headers headers = {
        {"Authorization", token}
    };

    char write_path[256];
    memset(write_path, 0, sizeof(write_path));;
    buildInfluxWritePath(write_path, sizeof(write_path), dbContext.org, dbContext.bucket);//////

    auto result = client.Post(write_path, headers, data, "application/octet-stream");
    if (result) {
        std::cout << result->status << std::endl;
        std::cout << result->body << std::endl;
    } else {
        auto err = result.error();
        std::cout << "Error: " << err << std::endl;
    }
}

void sendMeasurementToInfluxDB(const char* url, int port) {
	InfluxDBContext dbContext = {"Innoboat", "Innomaker", "gK8YfMaAl54lo2sgZLiM3Y5CQStHip-7mBe5vbhh1no86k72B4Hqo8Tj1qAL4Em-zGRUxGwBWLkQd1MR9foZ-g=="};
	char line_protocol_data[256];
    memset(line_protocol_data, 0, sizeof(line_protocol_data));
	setBucket(line_protocol_data, dbContext.bucket);
	addTag(line_protocol_data, "source", "BMS");
    //generate random values
    double value = 48 + (rand() % 6);
    addField(line_protocol_data, "teste", value);
	sendRequest(url, port, line_protocol_data, dbContext);
}



int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <url> <port>" << std::endl;
        return -1;
    }

    srand(time(NULL));
    while (true) {
        auto start = std::chrono::high_resolution_clock::now(); // Start timing
        sendMeasurementToInfluxDB(argv[1], atoi(argv[2]));
        auto end = std::chrono::high_resolution_clock::now(); // End timing
        std::chrono::duration<double, std::milli> elapsed = end - start; // Calculate elapsed time
        std::cout << "Loop execution time: " << elapsed.count() << " ms" << std::endl;
    }
    return 0;
}

int main2(int argc, char** argv) {
    Daly_BMS_UART bms(argv[1]);
    bms.Init();
    
    while ( true ) {
        auto start = std::chrono::high_resolution_clock::now(); // Start timing

        bool has_updated = bms.update();

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
