#include "../inc/ClientHandler.h"
#include <iostream>
#include <cstring>
#include <vector>
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

void RoomHandler::confirmACK() {
    isACK = true;
    std::lock_guard<std::mutex> lock(clientMutex);
}

uint64_t RoomHandler::generateMessageId() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
bool RoomHandler::waitForAck(uint64_t messageId, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            if (bufferACK.find(messageId) == bufferACK.end()) {
                return true;  // Подтверждение получено
            }
        }

        auto currentTime = std::chrono::steady_clock::now();
        int elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        if (elapsedMs > timeoutMs) {
            return false;  // Таймаут истёк
        }
    }
}


void RoomHandler::sendAcknowledgement(const uint64_t message_id,  SOCKADDR_IN serverAddr) const {
    // Формируем подтверждение и отправляем
    DataMessage ackMessage{};
    ackMessage.message_id = message_id;
    ackMessage.type = 3; // Тип подтверждения
    if (const int sendResult = sendto(sockfd, (char*)&ackMessage, sizeof(ackMessage), 0, (sockaddr*)&serverAddr, sizeof(serverAddr)); sendResult == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }
}

void RoomHandler::roomLoop() {
    std::cout << "Room " << roomKey << " is running..." << std::endl;

    while (isRunning) {
        if (isNewMessage) {
            if (messagePackage.dataPackage.type == 1) {  // Тип сообщения: аудио
                std::lock_guard<std::mutex> lock(clientMutex);

                // Создаём уникальный ID сообщения
                uint64_t messageId = generateMessageId();
                messagePackage.dataPackage.message_id = messageId;

                for (const auto& client : clients) {
                    if (client.second.username != messagePackage.dataPackage.username) {
                        bool ackReceived = false;
                        int retryCount = 0;
                        const int maxRetries = 3;      // Максимальное число попыток
                        const int timeoutMs = 1000;   // Таймаут ожидания подтверждения в миллисекундах

                        while (!ackReceived && retryCount < maxRetries) {
                            ssize_t bytesSent = sendto(sockfd, (char*)&messagePackage.dataPackage, sizeof(messagePackage.dataPackage), 0,
                                                       (sockaddr*)&client.second.clientAddr, sizeof(client.second.clientAddr));
                            if (bytesSent > 0) {
                                std::cout << "Message sent to client: " << client.second.username << ", message_id: " << messageId << std::endl;

                                // Ожидание подтверждения
                                ackReceived = waitForAck(messageId, timeoutMs);
                                if (!ackReceived) {
                                    retryCount++;
                                    std::cerr << "No acknowledgment received. Retrying (" << retryCount << "/" << maxRetries << ")..." << std::endl;
                                }
                            } else {
                                std::cerr << "Failed to send message to client: " << client.second.username
                                          << " Error: " << WSAGetLastError() << std::endl;
                                break;
                            }
                        }

                        if (!ackReceived) {
                            std::cerr << "Failed to receive acknowledgment from client: " << client.second.username << " after " << maxRetries << " attempts." << std::endl;
                        }
                    }
                }
            } else if (messagePackage.dataPackage.type == 3) {  // Тип сообщения: подтверждение (ACK)
                std::lock_guard<std::mutex> lock(clientMutex);
                uint64_t receivedId = messagePackage.dataPackage.message_id;
                if (bufferACK.find(receivedId) != bufferACK.end()) {
                    bufferACK.erase(receivedId);
                }
                std::cout << "Acknowledgment received for message_id: " << receivedId << std::endl;
            }

            isNewMessage = false;
        }
    }
}
