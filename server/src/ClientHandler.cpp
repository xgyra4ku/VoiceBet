#include "../inc/ClientHandler.h"
#include <iostream>
#include <cstring>
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов


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

void RoomHandler::addClient(const SOCKADDR_IN& clientAddr, DataMessage dataPackage) {
    ClientInformation newClient;
    newClient.username = dataPackage.username;
    newClient.clientAddr = clientAddr;
    std::lock_guard<std::mutex> lock(clientMutex);
    clients[dataPackage.username] = newClient;
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

void RoomHandler::confirmAKS() {
    isAKS = true;
    std::lock_guard<std::mutex> lock(clientMutex);
}

void RoomHandler::confirmMessage() {
    while (!isAKS) {

    }
}


void RoomHandler::roomLoop() {
    std::cout << "Room " << roomKey << " is running..." << std::endl;
    while (isRunning) {
        if (isNewMessage){
            //std::cout << "Message received in room " << roomKey << ": " << messagePackage.dataPackage.username << std::endl;

            // Пример отправки ответа
            std::lock_guard<std::mutex> lock(clientMutex);
            for (const auto& client : clients) {
                 if (client.second.username != messagePackage.dataPackage.username) {
                    ssize_t bytesSent = sendto(sockfd, (char*)&messagePackage.dataPackage, sizeof(messagePackage.dataPackage), 0,
                           (sockaddr*)&client.second.clientAddr, sizeof(client.second.clientAddr));
                    if (bytesSent > 0) {
                        std::cout << "Response sent to client: " << client.second.username << std::endl;
                    } else {
                        std::cerr << "Failed to send response to client: " << client.second.username
                                  << " Error: " << WSAGetLastError() << std::endl;
                    }
                 }

            }
            isNewMessage = false;
        }
    }
}
