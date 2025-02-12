#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

void sendResponse(SOCKET clientSocket, const std::string& content, const std::string& contentType) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;

    send(clientSocket, response.c_str(), response.length(), 0);
}

void sendFileContent(SOCKET clientSocket, const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        sendResponse(clientSocket, "404 Not Found", "text/plain");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string contentType = "text/plain";
    if (filename.find(".html") != std::string::npos) contentType = "text/html";
    else if (filename.find(".css") != std::string::npos) contentType = "text/css";

    sendResponse(clientSocket, buffer.str(), contentType);
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) return;

    buffer[bytesReceived] = '\0';
    std::string request(buffer);

    if (request.find("GET / ") != std::string::npos || request.find("GET /index.html") != std::string::npos) {
        sendFileContent(clientSocket, "index.html");
    } 
    else if (request.find("GET /style.css") != std::string::npos) {
        sendFileContent(clientSocket, "style.css");
    } 
    else if (request.find("GET /submit?msg=") != std::string::npos) {
        size_t msgPos = request.find("msg=") + 4;
        std::string message = request.substr(msgPos, request.find(" ", msgPos) - msgPos);

        std::ofstream file("messages.txt", std::ios::app);
        file << message << "\n";
        file.close();

        sendFileContent(clientSocket, "index.html");
    } 
    else {
        sendResponse(clientSocket, "404 Not Found", "text/plain");
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server running on port " << PORT << "...\n";

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            continue;
        }

        handleClient(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
