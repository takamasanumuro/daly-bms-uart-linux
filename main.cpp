#include <iostream>
#include "daly-bms-uart.h"
#include <time.h>
#include <chrono> // Include chrono for timing
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./libraries/cpp-httplib/httplib.h"
#include "./libraries/line-protocol/LineProtocol.hpp"

//Function to add each cell voltage to the line protocol based on number of cells
void addCellVoltagesToLineProtocol(char* line_protocol_data, Daly_BMS_UART bms) {
    int numberOfCells = bms.get.numberOfCells;
    if (numberOfCells < 1 || numberOfCells > 48) {
        return;
    }

    char voltages[2048] = {0};
    char buffer[256] = {0};

    for (int i = 0; i < bms.get.numberOfCells; i++) {
        char cellVoltage[32];
        memset(cellVoltage, 0, sizeof(cellVoltage));
        sprintf(cellVoltage, "cellVoltage%d", i + 1);
        addField(line_protocol_data, cellVoltage, bms.get.cellVmV[i] / 1000.0);
        sprintf(buffer, "\033[0mCell Voltage %d: \033[32;1m%.3f\n", i + 1, bms.get.cellVmV[i] / 1000.0);
        strcat(voltages, buffer);
    }

    printf("%s", voltages);
}

//!Function to add each field of BMS data to the line protocol
void addBMSDataToLineProtocol(char* line_protocol_data, Daly_BMS_UART bms) {
    addField(line_protocol_data, "packVoltage", bms.get.packVoltage);
    addField(line_protocol_data, "packCurrent", bms.get.packCurrent);
    addField(line_protocol_data, "packSOC", bms.get.packSOC);
    addField(line_protocol_data, "tempAverage", bms.get.tempAverage);
    addField(line_protocol_data, "maxCellmV", bms.get.maxCellmV);
    addField(line_protocol_data, "maxCellVNum", bms.get.maxCellVNum);
    addField(line_protocol_data, "minCellmV", bms.get.minCellmV);
    addField(line_protocol_data, "minCellVNum", bms.get.minCellVNum);
    addField(line_protocol_data, "numberOfCells", bms.get.numberOfCells);
    addField(line_protocol_data, "numOfTempSensors", bms.get.numOfTempSensors);
    addField(line_protocol_data, "bmsCycles", bms.get.bmsCycles);
    addField(line_protocol_data, "bmsHeartBeat", bms.get.bmsHeartBeat);
    addField(line_protocol_data, "disChargeFetState", bms.get.disChargeFetState);
    addField(line_protocol_data, "chargeFetState", bms.get.chargeFetState);
    addField(line_protocol_data, "resCapacitymAh", bms.get.resCapacitymAh);
    addCellVoltagesToLineProtocol(line_protocol_data, bms);
}


bool sendRequest(const char* url, int port, const char* data, InfluxDBContext dbContext) {
    httplib::Client client(url, port);
    std::string token = "Token " + std::string(dbContext.token);
    httplib::Headers headers = {
        {"Authorization", token},
        {"Connection", "keep-alive"},
    };

    char write_path[256];
    memset(write_path, 0, sizeof(write_path));;
    buildInfluxWritePath(write_path, sizeof(write_path), dbContext.org, dbContext.bucket);//////

    auto result = client.Post(write_path, headers, data, "application/octet-stream");
    if (result) {
        //std::cout << result->status << '\n';
        //std::cout << result->body << '\n';
    } else {
        auto err = result.error();
        std::cout << "Error: " << err << '\n';
    }
}

void sendMeasurementToInfluxDB(const char* url, int port, Daly_BMS_UART bms) {
	InfluxDBContext dbContext = {"Innoboat", "Innomaker", "gK8YfMaAl54lo2sgZLiM3Y5CQStHip-7mBe5vbhh1no86k72B4Hqo8Tj1qAL4Em-zGRUxGwBWLkQd1MR9foZ-g=="};
	char line_protocol_data[1024];
    memset(line_protocol_data, 0, sizeof(line_protocol_data));
	setBucket(line_protocol_data, dbContext.bucket);
	addTag(line_protocol_data, "source", "BMS");
    addBMSDataToLineProtocol(line_protocol_data, bms);
	sendRequest(url, port, line_protocol_data, dbContext);
}

void printBMSInfo(Daly_BMS_UART bms) {
    std::cout << "\033[0mbasic BMS Data:             \033[32;1m" << bms.get.packVoltage << "V, " << bms.get.packCurrent << "A, SOC " << bms.get.packSOC << "\% " << '\n';
    std::cout << "\033[0mPackage Temperature:        \033[32;1m" << bms.get.tempAverage << '\n';
    std::cout << "\033[0mHighest Cell Voltage Nr:    \033[32;1m" << bms.get.maxCellVNum << " with voltage " << (bms.get.maxCellmV / 1000) << '\n';
    std::cout << "\033[0mLowest Cell Voltage Nr:     \033[32;1m" << bms.get.minCellVNum << " with voltage " << (bms.get.minCellmV / 1000) << '\n';
    std::cout << "\033[0mNumber of Cells:            \033[32;1m" << bms.get.numberOfCells << '\n';
    std::cout << "\033[0mNumber of Temp Sensors:     \033[32;1m" << bms.get.numOfTempSensors << '\n';
    std::cout << "\033[0mBMS Chrg / Dischrg Cycles:  \033[32;1m" << bms.get.bmsCycles << '\n';
    std::cout << "\033[0mBMS Heartbeat:              \033[32;1m" << bms.get.bmsHeartBeat << '\n'; // cycle 0-255
    std::cout << "\033[0mDischarge MOSFet Status:    \033[32;1m" << bms.get.disChargeFetState << '\n';
    std::cout << "\033[0mCharge MOSFet Status:       \033[32;1m" << bms.get.chargeFetState << '\n';
    std::cout << "\033[0mRemaining Capacity mAh:     \033[32;1m" << bms.get.resCapacitymAh << '\n';
}

int main(int argc, char** argv) {

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <serial port> <influxdb url> <influxdb port>" << '\n';
        return -1;
    }

    const char* serial_port = argv[1];
    const char* url = argv[2];
    int port = atoi(argv[3]);

    Daly_BMS_UART bms(serial_port);
    bms.Init();
    
    while ( true ) {
        auto start = std::chrono::high_resolution_clock::now(); // Start timing

        bool has_updated = bms.update();
        sendMeasurementToInfluxDB(url, port, bms);
        printBMSInfo(bms);
        auto end = std::chrono::high_resolution_clock::now(); // End timing
        std::chrono::duration<double, std::milli> elapsed = end - start; // Calculate elapsed time
        std::cout << "\033[0mLoop execution time:        \033[32;1m" << elapsed.count() << " ms" << '\n';
        std::cout << "\033[0m\033[25A\033[0G";
        
    }
    return 0;
}
