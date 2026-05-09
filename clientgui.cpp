#define WEBUI_USE_STATIC_LIB
#include "webui.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <regex>

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
    #include <unistd.h>
    #define CLOSE_SOCKET close
    #define SOCKET_TYPE int
    #define SOCKET_ERROR -1
    #define INVALID_SOCKET -1
#endif

webui::window win;
SOCKET_TYPE clientSocket = INVALID_SOCKET;
std::string client_version = "v5.8.2026";

std::string strip_ansi(std::string str) {
    return std::regex_replace(str, std::regex("\x1B\\[[0-?]*[ -/]*[@-~]"), "");
}

void receiveMessages() {
    char buf[512];
    while (true) {
        int r = recv(clientSocket, buf, 511, 0);
        if (r > 0) {
            buf[r] = '\0';
            std::string msg(buf);
            
            size_t pos = msg.find("uni>");
            if (pos != std::string::npos) msg = msg.substr(pos + 4);

            msg = strip_ansi(msg);

            size_t start_pos = 0;
            while((start_pos = msg.find("'", start_pos)) != std::string::npos) {
                msg.replace(start_pos, 1, "\\'");
                start_pos += 2;
            }
            win.run("addMessage('" + msg + "')");
        } else {
            win.run("addMessage('Disconnected from server')");
            break;
        }
    }
}

void handle_connect(webui::window::event* e) {
    std::string ip = e->get_string(0);
    int port = (int)e->get_int(1);

    #ifdef _WIN32
        WSADATA w;
        WSAStartup(MAKEWORD(2, 2), &w);
    #endif

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        win.run("alert('Connection Failed')");
        return;
    }

    win.run("onConnected()");
    std::thread(receiveMessages).detach();
}

void handle_send(webui::window::event* e) {
    std::string msg = e->get_string(0);
    if (!msg.empty() && clientSocket != INVALID_SOCKET) {
        send(clientSocket, msg.c_str(), (int)msg.length(), 0);
    }
}

int main() {
    win.bind("connect_to_server", handle_connect);
    win.bind("send_message", handle_send);

    std::string html = R"HTML(
    <html>
    <head>
        <script src="webui.js"></script>
        <style>
            body { 
                background: #1e1e2e; 
                color: #cdd6f4; 
                font-family: sans-serif; 
                margin: 0; 
                padding: 20px; 
                display: flex; 
                flex-direction: column; 
                height: 100vh;
                box-sizing: border-box;
            }

            #login-screen { 
                display: flex; 
                flex-direction: column; 
                gap: 10px; /* This adds padding/spacing between all inputs and buttons */
                max-width: 300px;
                margin: auto;
            }

            #chat-screen { display: none; flex: 1; flex-direction: column; }
            #chat-box { 
                background: #181825; 
                flex: 1; 
                overflow-y: auto; 
                padding: 10px; 
                border: 1px solid #45475a; 
                margin-bottom: 10px; 
                border-radius: 5px; 
            }

            input { 
                background: #313244; 
                border: 1px solid #45475a; 
                color: white; 
                padding: 10px; 
                border-radius: 4px; 
            }

            button { 
                background: #ff4000; 
                color: white; 
                border: none; 
                padding: 10px; 
                cursor: pointer; 
                border-radius: 4px; 
            }

            button:hover { background: #ff5500; }

            .bottom-left {
                position: fixed;
                bottom: 15px;
                left: 15px;
                font-size: 0.75rem;
                color: #6c7086;
                pointer-events: none
            }

            .bottom-right {
                position: fixed;
                bottom: 15px;
                right: 15px;
                font-size: 0.75rem;
                color: #6c7086;
                pointer-events: none;
            }

            .msg { border-bottom: 1px solid #313244; padding: 5px; word-wrap: break-word; }
        </style>
    </head>
    <body>
        <div id="login-screen">
            <h2 style="color:#f38ba8; margin: 0 0 10px 0; text-align: center; width: 100%;">FoxChat</h2>
            <input type="text" id="ip" value="127.0.0.1" placeholder="IP Address">
            <input type="text" id="port" value="9020" placeholder="Port">
            <button onclick="connect_to_server(document.getElementById('ip').value, document.getElementById('port').value)">Connect</button>
            
            <div class="bottom-left">
                Apache License Version 2.0 - &copy; 2025 Jacob Meyers
            </div>
            <div class="bottom-right">
                FoxChat Client v5.8.2026
            </div>
        </div>

        <div id="chat-screen">
            <div id="chat-box"></div>
            <div style="display:flex; gap: 10px;">
                <input type="text" id="msg" style="flex:1;" placeholder="Type here..." onkeydown="if(event.key==='Enter') send()">
                <button onclick="send()">Send</button>
            </div>
        </div>

        <script>
            function onConnected() {
                document.getElementById('login-screen').style.display = 'none';
                document.getElementById('chat-screen').style.display = 'flex';
            }
            function addMessage(t) {
                const b = document.getElementById('chat-box');
                const d = document.createElement('div');
                d.className = 'msg';
                d.textContent = t; 
                b.appendChild(d);
                b.scrollTop = b.scrollHeight;
            }
            function send() {
                const i = document.getElementById('msg');
                if(i.value) {
                    send_message(i.value);
                    i.value = '';
                }
            }
        </script>
    </body>
    </html>)HTML";

    win.show(html);
    webui::wait();

    if (clientSocket != INVALID_SOCKET) CLOSE_SOCKET(clientSocket);
    #ifdef _WIN32
        WSACleanup();
    #endif
    return 0;
}