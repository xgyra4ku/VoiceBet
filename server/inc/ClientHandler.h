#ifndef ROOMHANDLER_H
#define ROOMHANDLER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <winsock2.h>
#include <map>

struct DataMessage {
    char username[128];      // имя пользователя
    char keyRoom[256];       // ключ канала
    char type;               // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 - запрос на подключение
    float audioBuffer[1024]; // Буфер для аудио данных
};

class RoomHandler {
private:
    struct MessagePackage {
        DataMessage dataPackage{};
        SOCKADDR_IN clientAddr{};
    };
    MessagePackage messagePackage{};
    std::string roomKey;                // Ключ комнаты
    std::vector<SOCKADDR_IN> clients;   // Список клиентов (адреса)
    std::thread roomThread;             // Поток обработки комнаты
    std::atomic<bool> isRunning;        // Флаг работы комнаты
    std::atomic<bool> isNewMessage;        // Флаг работы комнаты
    int sockfd;                         // Сокет сервера
    std::mutex clientMutex;             // Мьютекс для потокобезопасной работы с клиентами

    void roomLoop(); // Основной цикл обработки комнаты

public:
    RoomHandler(const std::string &key, int serverSockfd);

    ~RoomHandler();

    void addClient(const SOCKADDR_IN &clientAddr); // Добавить клиента
    void start(); // Запуск комнаты
    void stop(); // Остановка комнаты
    void processMessage(DataMessage dataPackage, SOCKADDR_IN clientAddr);
};

#endif
