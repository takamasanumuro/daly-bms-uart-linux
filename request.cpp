#include "./libraries/cpp-httplib/httplib.h"
#include "./libraries/line-protocol/LineProtocol.hpp"

bool sendRequest(const char* url, int port, const char* data, const char* bucket) {
    httplib::Client client(url, port);
    const char* token = std::getenv("INFLUXDB_TOKEN");
    if (token == nullptr) {
        std::cerr << "Error: INFLUXDB_TOKEN environment variable not set!" << std::endl;
        return false;
    }
    
    std::string token_auth = "Token " + std::string(token);
    httplib::Headers headers = {
        {"Authorization", token_auth},
        {"Connection", "keep-alive"},
    };

    char write_path[4096];
    memset(write_path, 0, sizeof(write_path));
    buildInfluxWritePath(write_path, sizeof(write_path), "Innomaker", bucket);

    httplib::Result result = client.Post(write_path, headers, data, "application/octet-stream");
    if (result) {
        // printf("Result status: %d\n", result->status);
        return true;
    } else {
        auto err = result.error();
        std::cout << "Error: " << err << '\n';
        return false;
    }
}

