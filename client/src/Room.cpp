#include "../inc/Room.h"
#include <cmath>
#include <iostream>
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов
#include <mutex>

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
    dataPackageMessage = dataPackage;
    isNewMessage = true;
    std::lock_guard<std::mutex> lock(clientMutex);

}

void Room::handleError(PaError err) {
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

void Room::roomLoop() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Создание UDP сокета для IPv4
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (isRunning) {
        if (isNewMessage) {
            if (isSignalAboveThreshold(dataPackageMessage.audioBuffer, FRAMES_PER_BUFFER)) {
                std::cout << "Signal above threshold, processing..." << std::endl;
                // Здесь можно обработать или отправить звук
                // Обработка аудио-данных
                PaError err = Pa_WriteStream(streamAudio, dataPackageMessage.audioBuffer, FRAMES_PER_BUFFER);
                if (err == paOutputUnderflowed) {
                    std::cerr << "PortAudio warning: Output underflowed." << std::endl;
                } else {
                    handleError(err);
                }// Формирование ответа клиенту
            } else {
                std::cout << "Signal below threshold, ignoring..." << std::endl;
                // Игнорируем звук, т.к. он слишком тихий
            }
            isNewMessage = false;
            std::lock_guard<std::mutex> lock(clientMutex);
        }
        // Чтение аудио данных из устройства
        handleError(Pa_ReadStream(recordAudio, dataPackageSend.audioBuffer, FRAMES_PER_BUFFER));

        // Логируем отправку данных
        int sendResult = sendto(sockfd, (char*)&dataPackageSend, sizeof(dataPackageSend), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            std::cout << dataPackageSend.type << std::endl;
        } else {
            std::cout << "Sent data to server" << std::endl; // Логирование отправки
        }
    }
}



