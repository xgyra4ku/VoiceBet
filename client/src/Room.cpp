#include "../inc/Room.h"
#include <cmath>
#include <iostream>
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов
#include <mutex>
#include <vector>

Room::Room(const std::string &key, int serverSockfd, DataMessage dataPackage, SOCKADDR_IN serverAddr)
    : serverAddr(serverAddr), roomKey(key), isRunning(false), isNewMessage(false) , sockfd(serverSockfd){

    // Инициализация PortAudio
    handleError(Pa_Initialize());  // Инициализация библиотеки PortAudio
    handleError(Pa_OpenDefaultStream(&streamAudio, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(streamAudio));  // Запуск потока


    handleError(Pa_OpenDefaultStream(&recordAudio, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(recordAudio));

    std::strcpy(dataPackageSend.username, dataPackage.username);  // Копируем имя пользователя в объект
    std::strcpy(dataPackageSend.keyRoom, dataPackage.keyRoom);  // Копируем ключ канала в объект
    dataPackageSend.type = dataPackage.type;  // Устанавливаем тип данных
    std::strcpy( dataPackageConfirm.username, dataPackage.username);  // Копируем имя пользователя в объект
    std::strcpy( dataPackageConfirm.keyRoom, dataPackage.keyRoom);  // Копируем ключ канала в объект
    dataPackageConfirm.type = 3;
}

Room::~Room() {
    stop();
}

void Room::start() {
    isRunning = true;
    roomThread = std::thread(&Room::roomLoop, this);
}

void Room::stop() {
    isRunning = false;
    if (roomThread.joinable()) {
        roomThread.join();
    }
}

void Room::processMessage(const DataMessage &dataPackage) {
    if (dataPackage.type == 3) {
        std::lock_guard<std::mutex> lock(clientMutex);
        uint64_t receivedId = dataPackageMessage.message_id;
        if (bufferACK.find(receivedId) != bufferACK.end()) {
            bufferACK.erase(receivedId);
        }
        std::cout << "Acknowledgment received for message_id: " << receivedId << std::endl;
    } else {
        dataPackageMessage = dataPackage;
        isNewMessage = true;
        std::lock_guard<std::mutex> lock(clientMutex);
    }
}

void Room::handleError(const PaError err) {
    if (err != paNoError) {
        // Если ошибка связана с недогрузкой, просто предупреждаем
        if (err == paOutputUnderflowed) {
            std::cerr << "PortAudio warning: Output underflowed. Continuing..." << std::endl;
            return; // Игнорируем ошибку
        }

        // Для других ошибок выводим сообщение и завершаем выполнение
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
bool Room::isSignalAboveThreshold(const float *buffer, int frames) const {

    float maxAmplitude = 0.0f;

    for (int i = 0; i < frames; ++i) {
        maxAmplitude = std::max(maxAmplitude, std::fabs(buffer[i]));
    }

    return maxAmplitude > THRESHOLD;  // Если максимальная амплитуда больше порога, то сигнал значимый
}
uint64_t Room::generateMessageId() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
bool Room::waitForAck(uint64_t messageId, int timeoutMs) {
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

void Room::sendAcknowledgement(const uint64_t message_id,  SOCKADDR_IN serverAddr) const {
    // Формируем подтверждение и отправляем
    DataMessage ackMessage{};
    ackMessage.message_id = message_id;
    ackMessage.type = 3; // Тип подтверждения
    std::memset(ackMessage.audioBuffer, 0, sizeof(ackMessage.audioBuffer));
    std::strncpy(ackMessage.username, "ack", sizeof(ackMessage.username) - 1);
    std::strncpy(ackMessage.keyRoom, "ack_room", sizeof(ackMessage.keyRoom) - 1);
    const int sendResult = sendto(sockfd, (char*)&ackMessage, sizeof(ackMessage), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sending ACK message. Type: " << ackMessage.type << ", ID: " << ackMessage.message_id << std::endl;
    }
}
// Функция для отправки данных серверу
void Room::sendData(const DataMessage &dataPackage, SOCKADDR_IN serverAddr) {
    uint64_t messageId = generateMessageId();
    dataPackageSend.message_id = messageId;
    int sendResult = sendto(sockfd, (char*)&dataPackageSend, sizeof(dataPackageSend), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult > 0) {
        std::cout << "Data sent successfully. Type: " << dataPackageSend.type << ", ID: " << messageId << std::endl;
    } else {
        std::cout << "Failed to send data to server. Error: " << WSAGetLastError() << std::endl;
    }
    //
    // bufferACK.insert(messageId);
    // // Логируем отправку данных
    // int sendResult = sendto(sockfd, (char*)&dataPackageSend, sizeof(dataPackageSend), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    // if (sendResult > 0) {
    //     bool ackReceived = false;
    //     int retryCount = 0;
    //     const int maxRetries = 3;      // Максимальное число попыток
    //     const int timeoutMs = 1000;   // Таймаут ожидания подтверждения в миллисекундах
    //
    //     while (!ackReceived && retryCount < maxRetries) {
    //         ssize_t bytesSent = sendto(sockfd, (char*)&dataPackageSend, sizeof(dataPackageSend), 0,
    //                                    (sockaddr*)&serverAddr, sizeof(serverAddr));
    //         if (bytesSent > 0) {
    //             std::cout << "Message sent to client: " << dataPackageSend.username << ", message_id: " << dataPackageSend.message_id << std::endl;
    //
    //             // Ожидание подтверждения
    //             ackReceived = waitForAck(messageId, timeoutMs);
    //             if (!ackReceived) {
    //                 retryCount++;
    //                 std::cerr << "No acknowledgment received. Retrying (" << retryCount << "/" << maxRetries << ")..." << std::endl;
    //             }
    //         } else {
    //             std::cerr << "Failed to send message to client: " << dataPackageSend.username
    //                       << " Error: " << WSAGetLastError() << std::endl;
    //             break;
    //         }
    //     }
    //
    //     if (!ackReceived) {
    //         std::cerr << "Failed to receive acknowledgment from client: " << dataPackageSend.username << " after " << maxRetries << " attempts." << std::endl;
    //     }
    // } else {
    //     std::cout << "Sent falied data to server" << std::endl; // Логирование отправки
    // }
}

void Room::roomLoop() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Создание UDP сокета для IPv4
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (isRunning) {
        // if (isNewMessage) {
        //     if (dataPackageMessage.type == 1) {
        //         if (bufferMessage.find(dataPackageMessage.message_id) == bufferMessage.end()){
        //             if (isSignalAboveThreshold(dataPackageMessage.audioBuffer, FRAMES_PER_BUFFER)) {
        //                 std::cout << "Signal above threshold, processing..." << std::endl;
        //                 // Здесь можно обработать или отправить звук
        //                 // Обработка аудио-данных
        //                 PaError err = Pa_WriteStream(streamAudio, dataPackageMessage.audioBuffer, FRAMES_PER_BUFFER);
        //                 if (err == paOutputUnderflowed) {
        //                     std::cerr << "PortAudio warning: Output underflowed." << std::endl;
        //                 } else {
        //                     handleError(err);
        //                 }// Формирование ответа клиенту
        //             } else {
        //                 std::cout << "Signal below threshold, ignoring..." << std::endl;
        //                 // Игнорируем звук, т.к. он слишком тихий
        //             }
        //             bufferMessage.clear();
        //             bufferMessage.insert(dataPackageMessage.message_id);
        //         }
        //         sendAcknowledgement(dataPackageMessage.message_id, serverAddr);
        //         std::lock_guard<std::mutex> lock(clientMutex);
        //     }
        //     isNewMessage = false;
        // }

        // Чтение аудио данных из устройства
        handleError(Pa_ReadStream(recordAudio, dataPackageSend.audioBuffer, FRAMES_PER_BUFFER));
        int sendResult = sendto(sockfd, (char*)&dataPackageSend, sizeof(dataPackageSend), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        sendData(dataPackageSend, serverAddr);
    }
}



