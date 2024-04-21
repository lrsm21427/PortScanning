cmake_minimum_required(VERSION 3.27)
project(PortScanning)

set(CMAKE_CXX_STANDARD 17)

add_executable(PortScanning main.cpp)

target_link_libraries(PortScanning ws2_32)