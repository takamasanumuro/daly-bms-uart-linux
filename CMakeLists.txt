cmake_minimum_required(VERSION 3.10)
project(dalybms)
#Add debug info
set(CMAKE_BUILD_TYPE Debug)
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    message(STATUS "OpenSSL found")
    set(HTTPLIB_IS_USING_OPENSSL TRUE)
endif()

include_directories(${CMAKE_SOURCE_DIR}/libraries)

file(GLOB LIB_SOURCES "libraries/line-protocol/*.cpp" "libraries/line-protocol/*.hpp")

add_library(line-protocol STATIC ${LIB_SOURCES})

add_executable(${PROJECT_NAME} main.cpp daly-bms-uart.cpp daly-bms-uart.h request.cpp request.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        line-protocol
        atomic
        $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::SSL>
        $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::Crypto>)

install(TARGETS dalybms RUNTIME DESTINATION bin)
