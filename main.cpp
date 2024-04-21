#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <winsock2.h>
#include <regex>
#pragma comment(lib, "ws2_32.lib")

std::mutex printMutex;
std::atomic<int> scannedPorts(0);

bool scanPort(const std::string& host, int port, int timeout) {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;

    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host.c_str());
    server.sin_port = htons(port);

    // 设置连接超时时间
    struct timeval tv;
    tv.tv_sec = timeout / 1000;  // 超时时间转换为秒
    tv.tv_usec = (timeout % 1000) * 1000;  // 超时时间的余数转换为微秒
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));

    iResult = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if (iResult == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return false;
    }

    closesocket(sock);
    WSACleanup();
    return true;
}

void scanRange(const std::string& host, int startPort, int endPort, int timeout) {
    for (int port = startPort; port <= endPort; ++port) {
        if (scanPort(host, port, timeout)) {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "Port " << port << " is open" << std::endl;
        } else {
            std::lock_guard<std::mutex> lock(printMutex);
            std::cout << "Port " << port << " is closed or not responding" << std::endl;
        }
        scannedPorts++;
    }
}

int main() {
    std::string userInput;
    std::cout << "Enter the target host and port range (format: ip startPort-endPort): ";
    std::getline(std::cin, userInput);

    std::regex pattern("scan\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s+(\\d+)-(\\d+)");
    std::smatch matches;

    if (std::regex_match(userInput, matches, pattern)) {
        std::string host = matches[1];
        int startPort = std::stoi(matches[2]);
        int endPort = std::stoi(matches[3]);
        int numThreads = std::thread::hardware_concurrency(); // Default to hardware concurrency
        int timeout = 200; // Default timeout set to 1000 milliseconds

        int portsPerThread = (endPort - startPort + 1) / numThreads;

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i) {
            int threadStartPort = startPort + i * portsPerThread;
            int threadEndPort = (i == numThreads - 1) ? endPort : threadStartPort + portsPerThread - 1;
            threads.emplace_back(scanRange, host, threadStartPort, threadEndPort, timeout);
        }

        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << "Scanned " << scannedPorts.load() << " ports." << std::endl;
    } else {
        std::cerr << "Invalid input format" << std::endl;
        return 1;
    }

    return 0;
}
