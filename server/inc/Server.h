#ifndef SERVER_H
#define SERVER_H

#include <cstring>   // Для работы с C-строками, например, memset
#include <thread>    // Для работы с многозадачностью (в случае использования потоков)
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <map>

#include "ClientHandler.h"

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

constexpr int SAMPLE_RATE = 44100;  // Частота дискретизации (частота аудио)
constexpr int FRAMES_PER_BUFFER =1024;  // Количество сэмплов в одном аудио-буфере
constexpr int PORT = 12345;  // Порт для связи с клиентом
// Структура для хранения данных
// struct DataMessage {
//     char username[128];                   // имя пользователя
//     char keyRoom[256];               // ключ канала
//     char type;                         // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 - запрос на подключение
//     float audioBuffer[FRAMES_PER_BUFFER];   // Буфер для аудио данных
// };

class Server {
    sockaddr_in serverAddr{}, clientAddr{};
    DataMessage dataPackage{};  // Создаем объект для хранения аудио данных
    int sockfd, result;

    std::map<std::string, RoomHandler*> rooms;

    static void handleError(PaError err);
public:
    Server();
    ~Server();

    void run();
};



#endif //SERVER_H
