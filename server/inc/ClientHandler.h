#ifndef ROOMHANDLER_H
#define ROOMHANDLER_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <winsock2.h>
#include <map>
#include <set>

struct DataMessage {
    char username[128];      // имя пользователя
    char keyRoom[256];       // ключ канала
    int type;               // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 - запрос на подключение
    float audioBuffer[1024]; // Буфер для аудио данных
    uint64_t message_id;
    bool is_ack;
};


struct ClientInformation {
    std::string username; // Имя пользователя
    SOCKADDR_IN clientAddr;
};

class RoomHandler {
private:
    struct MessagePackage {
        DataMessage dataPackage{};
        SOCKADDR_IN clientAddr{};
    };
    MessagePackage messagePackage{};
    std::string roomKey;                // Ключ комнаты
    //std::vector<SOCKADDR_IN> clients;   // Список клиентов (адреса)
    std::map<std::string, ClientInformation> clients;

    std::thread roomThread;             // Поток обработки комнаты
    std::atomic<bool> isRunning;        // Флаг работы комнаты
    std::atomic<bool> isNewMessage{};
    std::atomic<bool> isACK{};
    int sockfd;                         // Сокет сервера
    std::mutex clientMutex;             // Мьютекс для потокобезопасной работы с клиентами
    std::set<uint64_t> bufferACK;
    std::set<uint64_t> bufferMessage;
    std::mutex ackMutex;


    void roomLoop(); // Основной цикл обработки комнаты
    static uint64_t generateMessageId();
    bool waitForAck(uint64_t messageId, int timeoutMs);

    void sendAcknowledgement(uint64_t message_id, SOCKADDR_IN& serverAddr) const;

public:
    RoomHandler(const std::string &key, int serverSockfd);

    ~RoomHandler();

    void addClient(const SOCKADDR_IN& clientAddr, DataMessage dataPackage); // Добавить клиента
    void start(); // Запуск комнаты
    void stop(); // Остановка комнаты
    void processMessage(DataMessage dataPackage, SOCKADDR_IN& clientAddr);
    void confirmACK();
};

#endif
