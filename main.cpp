#include <iostream>
#include "daly-bms-uart.h"
#include <time.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./libraries/cpp-httplib/httplib.h"
#include "./libraries/line-protocol/LineProtocol.hpp"

// Shared atomic variable for watchdog
std::atomic<std::chrono::steady_clock::time_point> last_update_time;

// Signal handler for graceful termination
void signalHandler(int signum) {
    std::cout << "Terminating program due to watchdog timeout." << std::endl;
    std::exit(signum);
}

// Watchdog thread function
void watchdog(int timeout_seconds) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Check every second
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time.load());

        if (elapsed.count() > timeout_seconds) {
            std::cout << "Watchdog timeout! No updates for " << timeout_seconds << " seconds.\n";
            std::raise(SIGTERM); // Trigger termination signal
        }
    }
}

void addCellVoltagesToLineProtocol(char* line_protocol_data, Daly_BMS_UART bms, const char *prefix = "") {
    int numberOfCells = bms.get.numberOfCells;
    if (numberOfCells < 1 || numberOfCells > 48) {
        return;
    }

    char voltages[4096] = {0};
    char buffer[4096] = {0};

    for (int i = 0; i < numberOfCells; i++) {
        char cellVoltage[32];
        memset(cellVoltage, 0, sizeof(cellVoltage));
        sprintf(cellVoltage, "cellVoltage%d", i + 1);
        addField(line_protocol_data, std::string(prefix).append(cellVoltage).c_str(), bms.get.cellVmV[i] / 1000.0);
        sprintf(buffer, "\033[0m%sCell Voltage %d: \033[32;1m%.3f\n", prefix, i + 1, bms.get.cellVmV[i] / 1000.0);
        strcat(voltages, buffer);
    }

    printf("%s", voltages);
}

void addBMSDataToLineProtocol(char* line_protocol_data, Daly_BMS_UART bms, const char *prefix = "") {
    addField(line_protocol_data, std::string(prefix).append("Voltage").c_str(), bms.get.packVoltage);
    addField(line_protocol_data, std::string(prefix).append("Current").c_str(), bms.get.packCurrent);
    addField(line_protocol_data, std::string(prefix).append("SOC").c_str(), bms.get.packSOC);
    addField(line_protocol_data, std::string(prefix).append("AvgTemperature").c_str(), bms.get.tempAverage);
    addField(line_protocol_data, std::string(prefix).append("DischargeFETState").c_str(), bms.get.disChargeFetState);
    addField(line_protocol_data, std::string(prefix).append("ChargeFETState").c_str(), bms.get.chargeFetState);
    addField(line_protocol_data, std::string(prefix).append("ResidualCapacitymAh").c_str(), bms.get.resCapacitymAh);
    addCellVoltagesToLineProtocol(line_protocol_data, bms, prefix);
}

bool sendRequest(const char* url, int port, const char* data, InfluxDBContext dbContext) {
    httplib::Client client(url, port);
    std::string token = "Token " + std::string(dbContext.token);
    httplib::Headers headers = {
        {"Authorization", token},
        {"Connection", "keep-alive"},
    };

    char write_path[4096];
    memset(write_path, 0, sizeof(write_path));
    buildInfluxWritePath(write_path, sizeof(write_path), dbContext.org, dbContext.bucket);

    auto result = client.Post(write_path, headers, data, "application/octet-stream");
    if (result) {
        return true;
    } else {
        auto err = result.error();
        std::cout << "Error: " << err << '\n';
        return false;
    }
}

void sendMeasurementToInfluxDB(const char* url, int port, Daly_BMS_UART bms, const char* battery_name) {
    InfluxDBContext dbContext = {"Innoboat", "Innomaker", "gK8YfMaAl54lo2sgZLiM3Y5CQStHip-7mBe5vbhh1no86k72B4Hqo8Tj1qAL4Em-zGRUxGwBWLkQd1MR9foZ-g=="};
    char line_protocol_data[4096];
    memset(line_protocol_data, 0, sizeof(line_protocol_data));
    setBucket(line_protocol_data, dbContext.bucket);
    addTag(line_protocol_data, "source", battery_name);
    if (strcmp(battery_name, "bateria-bombordo") == 0) {
        addBMSDataToLineProtocol(line_protocol_data, bms, "[BB]");
    } else if (strcmp(battery_name, "bateria-boreste") == 0) {
        addBMSDataToLineProtocol(line_protocol_data, bms, "[BO]");
    } else {
        addBMSDataToLineProtocol(line_protocol_data, bms, "");
    }
    sendRequest(url, port, line_protocol_data, dbContext);
}

void printBMSInfo(Daly_BMS_UART bms, const char* battery_name) {
    std::cout << "\033[0mBattery Name: \033[32;1m" << battery_name << '\n';
    std::cout << "\033[0mBMS Data: \033[32;1m" << bms.get.packVoltage << "V, " << bms.get.packCurrent << "A, SOC " << bms.get.packSOC << "% " << '\n';
    std::cout << "\033[0mTemperature: \033[32;1m" << bms.get.tempAverage << '\n';
    std::cout << "\033[0mHighest Cell Voltage: \033[32;1m" << bms.get.maxCellVNum << " with voltage " << (bms.get.maxCellmV / 1000.0) << '\n';
    std::cout << "\033[0mLowest Cell Voltage: \033[32;1m" << bms.get.minCellVNum << " with voltage " << (bms.get.minCellmV / 1000.0) << '\n';
    std::cout << "\033[0mDischarge FET Status: \033[32;1m" << bms.get.disChargeFetState << '\n';
    std::cout << "\033[0mCharge FET Status: \033[32;1m" << bms.get.chargeFetState << '\n';
    std::cout << "\033[0mRemaining Capacity mAh: \033[32;1m" << bms.get.resCapacitymAh << '\n';
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cout << "Usage: " << argv[0] << " <serial_port> <battery_name> <url> <port>" << std::endl;
        return -1;
    }

    const char* serial_port = argv[1];
    const char* battery_name = argv[2];
    const char* url = argv[3];
    int port = std::stoi(argv[4]);

    // Set up signal handler
    std::signal(SIGTERM, signalHandler);

    // Start watchdog thread
    last_update_time = std::chrono::steady_clock::now();
    std::thread watchdog_thread(watchdog, 10); // 10 seconds timeout

    Daly_BMS_UART bms(serial_port);
    bms.Init();

    while (true) {
        auto start = std::chrono::high_resolution_clock::now();

        bool has_updated = bms.update();
        if (!has_updated) {
            std::cout << "\033[0mFailed to update BMS data\n";
            continue;
        }

        sendMeasurementToInfluxDB(url, port, bms, battery_name);
        printBMSInfo(bms, battery_name);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "\033[0mLoop execution time: \033[32;1m" << elapsed.count() << " ms\n";

        // Update the last activity time
        last_update_time = std::chrono::steady_clock::now();
    }

    watchdog_thread.join();
    return 0;
}
