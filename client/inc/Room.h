#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <winsock2.h>
#include <map>
#include <portaudio.h>  // Для работы с аудио
#include <set>

#include "structMessage.h"

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

class Room {
    sockaddr_in serverAddr{};                // Структура для хранения адреса сервера
    std::string roomKey;                    // Ключ комнаты
    std::thread roomThread;                // Поток обработки комнаты
    std::atomic<bool> isRunning;          // Флаг работы комнаты
    std::atomic<bool> isNewMessage;      // Флаг работы комнаты
    std::mutex clientMutex;            // Мьютекс для потокобезопасной работы с клиентами
    std::set<uint64_t> bufferACK;
    std::set<uint64_t> bufferMessage;

    DataMessage dataPackageMessage{}, dataPackageSend{}, dataPackageConfirm{};

    PaStream* streamAudio = nullptr;   // Поток для трансляции аудио
    PaStream* recordAudio = nullptr;  // Поток для записи аудио

    int sockfd;// Сокет сервера
    const char* SERVER_IP = "192.168.0.107"; // IP-адрес сервера (localhost)
    const float THRESHOLD = 0.0f;            // Порог срабатывания для аудио (например, амплитуда должна быть больше 0.01)
    const int SAMPLE_RATE = 44100;            // Частота дискретизации (частота аудио)


    void roomLoop();
    bool isSignalAboveThreshold(const float *buffer, int frames) const;

    static uint64_t generateMessageId();

    bool waitForAck(uint64_t messageId, int timeoutMs);

    static void handleError(PaError err);
    void sendAcknowledgement(uint64_t message_id, SOCKADDR_IN serverAddr) const;

    void sendData(const DataMessage &dataPackage, SOCKADDR_IN serverAddr);

public:
    Room(const std::string &key, int serverSockfd, DataMessage dataPackage, SOCKADDR_IN serverAddr);
    ~Room();
    void stop();
    void start();
    void processMessage(const DataMessage &dataPackage);
};

#endif //ROOM_H
