#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define CLOSE_SOCKET closesocket
#define SOCKET_TYPE SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define CLOSE_SOCKET close
#define SOCKET_TYPE int
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif
#include <cstdio>
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <limits>


SOCKET_TYPE clientSocket;
std::vector<std::string> client_chat_history;
const char * deticatedClearMode = "clear";
void updScreen() {
    for (const auto& line : client_chat_history) printf("%s\n", line.c_str());
    printf("\e[38;5;248mMessage ~> ");
    fflush(stdout);
}
void receiveMessages() {
    char buf[512];
    int r = recv(clientSocket, buf, 511, 0);
    while (r > 0) {
        buf[r] = '\0';
        std::string msg(buf);
        size_t pos = msg.find("uni>");
        if (pos != std::string::npos) {
            msg = msg.substr(pos + 4);
        } 
        client_chat_history.push_back(msg);
        system(deticatedClearMode);
        updScreen();
        r = recv(clientSocket, buf, 511, 0);
    }
    printf("\nDisconnected from server\n");
}
int main() {
    #ifdef _WIN32
        deticatedClearMode = "cls";
    #endif
    std::string sel_address = "127.0.0.1";
    std::string sel_port_temp = "9020";
    printf("\e[0;31mFoxChat Client v12.14.2025\e[0;37m\n\n");
    printf("Server Address ~> ");
    fflush(stdout);
    std::getline(std::cin, sel_address);
    int sel_port = std::stoi(sel_port_temp);
    printf("People will be able to see your ip, mask this with a VPN (ALWAYS USE VPN!) Continue Y/n ~> ");
    fflush(stdout);
    std::string escape_novpn;
    std::getline(std::cin, escape_novpn);
    if (escape_novpn == "n" || escape_novpn == "N") exit(0);
    
    #ifdef _WIN32
        WSADATA w;
        WSAStartup(MAKEWORD(2,2), &w);
    #endif
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons(sel_port);
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result;
    if (getaddrinfo(sel_address.c_str(), nullptr, &hints, &result) != 0) {
        printf("Failed to resolve hostname\n");
        CLOSE_SOCKET(clientSocket);
    #ifdef _WIN32
        WSACleanup();
    #endif
        return 1;
    }
    a.sin_addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
    freeaddrinfo(result);
    if (connect(clientSocket, (sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) {
        printf("Connection failed\n");
        CLOSE_SOCKET(clientSocket);
    #ifdef _WIN32
        WSACleanup();
    #endif
        return 1;
    }
    printf("Connected to server\n");
    system(deticatedClearMode);
    printf("Message ~> \e[38;5;248m");
    fflush(stdout);
    std::thread recvThread(receiveMessages);
    recvThread.detach();
    std::string inputLine;
    while (true) {
        if (!std::getline(std::cin, inputLine)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            system(deticatedClearMode);
            updScreen();
            continue;
        }
        system(deticatedClearMode);
        if (inputLine.length() > 0 && inputLine.length() <= 512) {
            send(clientSocket, inputLine.c_str(), inputLine.length(), 0);
        } else updScreen();
    }
    CLOSE_SOCKET(clientSocket);
#ifdef _WIN32
    WSACleanup();
#endif
}
