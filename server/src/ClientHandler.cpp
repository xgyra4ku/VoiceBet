#include "../inc/ClientHandler.h"
#include <iostream>
#include <cstring>

RoomHandler::RoomHandler(const std::string& key, int serverSockfd)
    : roomKey(key), sockfd(serverSockfd), isRunning(false) {}

RoomHandler::~RoomHandler() {
    stop();
}

void RoomHandler::processMessage(DataMessage dataPackage, SOCKADDR_IN clientAddr) {
    messagePackage.dataPackage = dataPackage;
    messagePackage.clientAddr = clientAddr;
    isNewMessage = true;
    std::lock_guard<std::mutex> lock(clientMutex);
}

void RoomHandler::addClient(const SOCKADDR_IN& clientAddr) {
    std::lock_guard<std::mutex> lock(clientMutex);
    clients.push_back(clientAddr);
    std::cout << "Client added to room " << roomKey << std::endl;
}

void RoomHandler::start() {
    isRunning = true;
    roomThread = std::thread(&RoomHandler::roomLoop, this);
}

void RoomHandler::stop() {
    isRunning = false;
    if (roomThread.joinable()) {
        roomThread.join();
    }
}

void RoomHandler::roomLoop() {
    std::cout << "Room " << roomKey << " is running..." << std::endl;
    while (isRunning) {
        if (isNewMessage){
            std::cout << "Message received in room " << roomKey << ": " << messagePackage.dataPackage.username << std::endl;

            // Пример отправки ответа
            for (const auto& client : clients) {
                //sendto(sockfd, buffer, bytesReceived, 0,);
                ssize_t bytesSent = sendto(sockfd, (char*)&messagePackage.dataPackage, sizeof(messagePackage.dataPackage), 0, (sockaddr*)&client, sizeof(client));
                if (bytesSent > 0) {
                    std::cout << "Response sent to client." << std::endl;
                } else {
                    perror("Failed to send response");
                }
            }
            isNewMessage = false;
            std::lock_guard<std::mutex> lock(clientMutex);
        }
    }
}
