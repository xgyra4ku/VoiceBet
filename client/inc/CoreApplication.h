#ifndef COREAPPLICATION_H
#define COREAPPLICATION_H
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов
#include <mutex>

#include "structMessage.h"
#include "Room.h"

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock


class CoreApplication {
    sockaddr_in serverAddr{}, clientAddr{};              // Структура для хранения адреса сервера

    DataMessage dataReceive{}, dataSend{};                  // Создаем объект для хранения данных для отправки и получения

    PaStream* streamAudio = nullptr;   // Поток для трансляции аудио
    PaStream* recordAudio = nullptr;  // Поток для записи аудио

    Room* room = nullptr;

    const char* SERVER_IP = "192.168.0.107";        // IP-адрес сервера (localhost)
    const int PORT = 12345;                        // Порт для связи с сервером
    const char USERNAME[128] = "username342";       // Имя пользователя
    const char KEY_ROOM[256] = "1234";           // Ключ канала
    const char type = 1;                        // 1 - аудио включено, 0 - выключено
    const float THRESHOLD = 0.0f;            // Порог срабатывания для аудио (например, амплитуда должна быть больше 0.01)
    const int SAMPLE_RATE = 44100;            // Частота дискретизации (частота аудио)

    int sockfd, clientAddrLen;

    static void handleError(PaError err);
    bool isSignalAboveThreshold(const float* buffer, int frames);
public:
    CoreApplication();
    ~CoreApplication();
    void run();
};



#endif //COREAPPLICATION_H
