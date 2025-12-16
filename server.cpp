#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <string>
#include <cstring>
#include <random>
#include <set>
#include <chrono>
#include <map>
#include <fstream>
#include <iostream>
#include <vector>
#include "nlohmann/json.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using sock_t = SOCKET;
#define CLOSESOCK closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using sock_t = int;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSESOCK close
#endif

std::string serverVersion = "v12.15.2025";

static int lenlimit = 200;
static int max_clients_connected = 100;
std::vector<sock_t> clientSockets;
std::mutex clientMutex;
std::set<int> usedCodes;
std::mutex codeMutex;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(10000, 99999);
nlohmann::json serverConfigJson;
static int port = 9020;

int generateUniqueCode() {
    std::lock_guard<std::mutex> lock(codeMutex);
    int code;
    do {
        code = dis(gen);
    } while (usedCodes.count(code));
    usedCodes.insert(code);
    return code;
}

void releaseCode(int code) {
    std::lock_guard<std::mutex> lock(codeMutex);
    usedCodes.erase(code);
}

void broadcastMessage(const char* message, int len, sock_t senderSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (sock_t sock : clientSockets) {
        if (sock != senderSocket) send(sock, message, len, 0);
    }
}

void broadcastPrivateMessage(const char* message, int len, sock_t senderSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (sock_t sock : clientSockets) {
        if (sock == senderSocket) send(sock, message, len, 0);
    }
}

std::string replaceAll(
    std::string str,
    const std::vector<std::vector<std::string>>& vars
) {
    for (const auto& v : vars) {
        if (v.size() != 2 || v[0].empty()) continue;

        const std::string& from = v[0];
        const std::string& to   = v[1];

        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
    return str;
}

std::string clientIp;
std::string clientIpRaw;

std::vector<std::vector<std::string>> serverConfigApiVars;
void updateServerConfigApiVars() {
    serverConfigApiVars = {
        {"{clientip_id}", clientIp},
        {"{clientip_raw}", (std::string)clientIpRaw},
        {"{custom_port}", std::to_string(port)},

        {"{tColor_red}", "\e[0;31m"},
        {"{tColor_yellow}", "\e[0;33m"},
        {"{tColor_green}", "\e[0;32m"},
        {"{tColor_blue}", "\e[0;34m"},
        {"{tColor_purple}", "\e[0;35m"},
        {"{tColor_gray}", "\e[38;5;248m"},
        {"{tColor_reset}", "\e[0;37m"},
    };
}

void handleClient(sock_t clientSocket, const char* clientIpRawIn) {
    clientIpRaw = clientIpRawIn;
    int uniqueCode = generateUniqueCode();
    clientIp = clientIpRaw + "_" + std::to_string(uniqueCode);
    if (clientIpRaw == serverConfigJson["admin_username_ip"]) clientIp = "\033[31mAdmin\033[0m_" + std::to_string(uniqueCode);

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        clientSockets.push_back(clientSocket);
    }

    size_t connectedCount;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        connectedCount = clientSockets.size() - 1;
    }

    printf("Client %s connected\n", clientIp.c_str());
    std::string connected_message = clientIp + " has joined the chat! --> " + std::to_string(connectedCount) + " others connected.";
    broadcastMessage(connected_message.c_str(), connected_message.size(), clientSocket);
    
    updateServerConfigApiVars();
    std::string connected_message_private = replaceAll(serverConfigJson["welcome_message"].get<std::string>(), serverConfigApiVars);
    //std::string connected_message_private = "\033[33mYou (" + clientIp + "\033[33m) have joined the chat! --> " + std::to_string(connectedCount) + " others connected.\033[0m";
    broadcastPrivateMessage(connected_message_private.c_str(), connected_message_private.size(), clientSocket);
    

    char buf[512];
    std::chrono::steady_clock::time_point lastMsgTime = std::chrono::steady_clock::now();
    const int minDelayMs = serverConfigJson["message_cooldown_milliseconds"].get<int>();

    while (true) {
        int r = recv(clientSocket, buf, sizeof(buf), 0);
        if (r > 0) {
            auto now = std::chrono::steady_clock::now();
            auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMsgTime).count();
            if (deltaMs < minDelayMs) {
                // Ignore message if sent too quickly
                std::string spamWarningMessage  = replaceAll(serverConfigJson["spam_warning_message"].get<std::string>(), serverConfigApiVars);
                send(clientSocket, spamWarningMessage.c_str(), spamWarningMessage.size(), 0);
                continue;
            }
            lastMsgTime = now;

            if (r > lenlimit) {
                printf("Client %s sent too large message (%d bytes). Ignored.\n", clientIp.c_str(), r);
            } else {
                updateServerConfigApiVars();
                std::string message_prefix_others = replaceAll(serverConfigJson["message_prefix_others"].get<std::string>(), serverConfigApiVars);
                broadcastMessage((message_prefix_others + std::string(buf, r)).c_str(), message_prefix_others.size() + r, clientSocket);
                
                std::string message_prefix_self = replaceAll(serverConfigJson["message_prefix_self"].get<std::string>(), serverConfigApiVars);
                broadcastPrivateMessage((message_prefix_self + std::string(buf, r) + "\e[0m").c_str(), message_prefix_self.size() + r + 5, clientSocket);
            }
        } else {
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
        if (it != clientSockets.end()) clientSockets.erase(it);
    }

    releaseCode(uniqueCode);

    size_t remaining;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        remaining = clientSockets.size() - 1;
    }

    printf("Client %s disconnected\n", clientIp.c_str());
    std::string disconnected_message = clientIp + " has left the chat :( --> " + std::to_string(remaining) + " others connected.";
    broadcastMessage(disconnected_message.c_str(), disconnected_message.size(), clientSocket);

    CLOSESOCK(clientSocket);
}


int main() {
    std::ifstream f("serverconfig.json");
    if (!f.is_open()) {
        std::cerr << "Error: Could not open the \'serverconfig.json\' file." << std::endl;
        return 1;
    }
    serverConfigJson = nlohmann::json::parse(f);
    port = serverConfigJson["custom_port"].get<int>();
    int lenlimit = std::clamp(serverConfigJson["message_length_limit"].get<int>(), 0, 511);

    if (!serverConfigJson["license_agreement_FoxChat"] || !serverConfigJson["license_agreement_nlohmann-json"]) {
        printf("By downloading, using, or referencing FoxChat, you acknowledge that the creator is not responsible or liable for any illegal activity, misuse, or damages resulting from the use of this software.\n"
            "Use this software at your own risk.\n\n");
        printf("Confirm Agreement? (type \"YES\" to confirm) ~> ");
        std::string agreed;
        std::getline(std::cin, agreed);
        if (agreed != "YES") { printf("\nFailed to confirm agreement!"); return 0; }
    }

    if (!serverConfigJson["license_agreement_FoxChat"]) {
        std::string line;
        std::ifstream MyReadFile("server_licenses/FoxChat_LICENSE");
        if (MyReadFile.is_open()) {
            while (getline(MyReadFile, line)) {
                std::cout << line << std::endl;
            }
            MyReadFile.close();
        } else {
            std::cerr << "Error: Unable to open the (./server_licenses/license_agreement_FoxChat) FoxChat License file, redownload from source (do not rename the file)" << std::endl;
            return 1;
        }

        printf("\nConfirm Agreement to The FoxChat (Apache License Version 2.0) License? (type \"YES\" to confirm) ~> ");
        std::string agreed;
        std::getline(std::cin, agreed);
        if (agreed != "YES") { printf("\nFailed to confirm agreement to the FoxChat License!"); return 0; }
        serverConfigJson["license_agreement_FoxChat"] = true;
    } 
    if (!serverConfigJson["license_agreement_nlohmann-json"]) {
        std::string line;
        std::ifstream MyReadFile("server_licenses/nlohmann-json_LICENSE");
        if (MyReadFile.is_open()) {
            while (getline(MyReadFile, line)) {
                std::cout << line << std::endl;
            }
            MyReadFile.close();
        } else {
            std::cerr << "Error: Unable to open the (./server_licenses/license_agreement_nlohmann-json) nlohmann-json License file, redownload from source (do not rename the file)" << std::endl;
            return 1;
        }

        printf("\nConfirm Agreement to The nlohmann-json (MIT License) License? (type \"YES\" to confirm) ~> ");
        std::string agreed;
        std::getline(std::cin, agreed);
        if (agreed != "YES") { printf("\nFailed to confirm agreement to the nlohmann-json License!"); return 0; }
        serverConfigJson["license_agreement_nlohmann-json"] = true;
    } 
    std::ofstream out("serverconfig.json");
    out << serverConfigJson.dump(4);
    out.close();
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif


#ifdef _WIN32
    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);
#endif

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return 1;

    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    a.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) return 1;
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) return 1;

    printf(("FoxChat Server - " + serverVersion + "\n\n").c_str());
    printf("Server listening on port %d...\n", port);

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrSize = sizeof(clientAddr);
        sock_t clientS = accept(s, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientS == INVALID_SOCKET) continue;
        if (clientSockets.size() >= max_clients_connected) continue;
        const char* client_ip = inet_ntoa(clientAddr.sin_addr);
        std::thread(handleClient, clientS, client_ip).detach();
    }

    CLOSESOCK(s);
#ifdef _WIN32
    WSACleanup();
#endif
}
