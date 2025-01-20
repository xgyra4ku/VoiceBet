#include <iostream>  // Для вывода на экран
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

constexpr int SAMPLE_RATE = 44100;  // Частота дискретизации (частота аудио)
constexpr int FRAMES_PER_BUFFER = 1024;  // Количество сэмплов в одном аудио-буфере
constexpr const char* SERVER_IP = "127.0.0.1";  // IP-адрес сервера (localhost)
constexpr int PORT = 12345;  // Порт для связи с сервером

// Структура для хранения аудио данных
struct AudioData {
    float buffer[FRAMES_PER_BUFFER];  // Буфер для аудио данных
};

// Функция для обработки ошибок PortAudio
void handleError(PaError err) {
    if (err != paNoError) {  // Проверка на ошибку
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;  // Вывод ошибки в консоль
        std::exit(EXIT_FAILURE);  // Завершаем программу с ошибкой
    }
}

int main() {
    // Инициализация Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);  // Инициализация библиотеки Winsock версии 2.2
    if (result != 0) {  // Если инициализация не удалась
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return -1;  // Выход из программы с ошибкой
    }

    // Создание UDP сокета
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Создаем UDP сокет
    if (sockfd == INVALID_SOCKET) {  // Если создание сокета не удалось
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();  // Завершаем работу с Winsock
        return -1;  // Выход с ошибкой
    }

    sockaddr_in serverAddr{};  // Структура для хранения адреса сервера
    serverAddr.sin_family = AF_INET;  // Используем IPv4
    serverAddr.sin_port = htons(PORT);  // Устанавливаем порт (переводим в сетевой порядок байтов)

    // Преобразуем IP-адрес в двоичный формат
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {  // Проверка валидности IP-адреса
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        closesocket(sockfd);  // Закрытие сокета
        WSACleanup();  // Очистка Winsock
        return -1;  // Выход с ошибкой
    }

    // Инициализация PortAudio
    handleError(Pa_Initialize());  // Инициализация библиотеки PortAudio
    PaStream* stream;  // Поток для аудио
    handleError(Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(stream));  // Запуск потока

    std::cout << "Streaming audio to " << SERVER_IP << ":" << PORT << "..." << std::endl;

    AudioData data{};  // Создаем объект для хранения аудио данных
    while (true) {
        // Чтение аудио данных из устройства
        handleError(Pa_ReadStream(stream, data.buffer, FRAMES_PER_BUFFER));

        // Логируем отправку данных
        int sendResult = sendto(sockfd, (char*)&data, sizeof(data), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        } else {
            std::cout << "Sent data to server" << std::endl; // Логирование отправки
        }

    }

    // Завершение работы
    handleError(Pa_StopStream(stream));  // Остановка потока
    handleError(Pa_CloseStream(stream));  // Закрытие потока
    Pa_Terminate();  // Завершение работы с PortAudio

    // Закрытие сокета и очистка Winsock
    closesocket(sockfd);
    WSACleanup();

    return 0;
}
