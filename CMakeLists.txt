cmake_minimum_required(VERSION 3.10)
project(dalybms)
#Add debug info
set(CMAKE_BUILD_TYPE Debug)
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    message(STATUS "OpenSSL found")
    set(HTTPLIB_IS_USING_OPENSSL TRUE)
endif()


add_executable(${PROJECT_NAME} main.cpp daly-bms-uart.cpp daly-bms-uart.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::SSL>
        $<$<BOOL:${HTTPLIB_IS_USING_OPENSSL}>:OpenSSL::Crypto>)


install(TARGETS dalybms RUNTIME DESTINATION bin)
