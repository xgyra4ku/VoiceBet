#include <iostream>  // Для вывода на экран
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов
#include <math.h>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

constexpr int SAMPLE_RATE = 44100;  // Частота дискретизации (частота аудио)
constexpr int FRAMES_PER_BUFFER = 1024;  // Количество сэмплов в одном аудио-буфере
constexpr const char* SERVER_IP = "192.168.0.107";  // IP-адрес сервера (localhost)
constexpr int PORT = 12345;  // Порт для связи с сервером
constexpr char USERNAME[128] = "username2";  // Имя пользователя
constexpr char KEY_ROOM[256] = "1234";  // Ключ канала
constexpr char type = 1;  // 1 - аудио включено, 0 - выключено
// Порог срабатывания для аудио (например, амплитуда должна быть больше 0.01)
constexpr float THRESHOLD = 0.015f;
bool isNewAudio =false;
// Структура для хранения данных
struct DataMessage {
    char username[128];                       // имя пользователя
    char keyRoom[256];                       // ключ канала
    char type;                              // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 запрос на подключение
    float audioBuffer[FRAMES_PER_BUFFER];  // Буфер для аудио данных
};
sockaddr_in clientAddr{};
int clientAddrLen = sizeof(clientAddr);
int sockfd;
DataMessage data{};  // Создаем объект для хранения аудио данных
PaStream* stream2;  // Поток для аудио
std::thread roomThread;
std::mutex clientMutex;
// Функция для обработки ошибок PortAudio
void handleError(PaError err) {
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
// Функция для проверки уровня сигнала
bool isSignalAboveThreshold(const float* buffer, int frames) {
    float maxAmplitude = 0.0f;

    for (int i = 0; i < frames; ++i) {
        maxAmplitude = std::max(maxAmplitude, std::fabs(buffer[i]));
    }

    return maxAmplitude > THRESHOLD;  // Если максимальная амплитуда больше порога, то сигнал значимый
}

void streamAudioThread() {
    std::cout << "Thread is running" << std::endl;
    while (true) {
            // Получение ответа от сервера
            int bytesReceived = recvfrom(sockfd, (char*)&data, sizeof(data), 0, (sockaddr*)&clientAddr, &clientAddrLen);
            if (bytesReceived > 0) {

                if (isSignalAboveThreshold(data.audioBuffer, FRAMES_PER_BUFFER)) {
                    std::cout << "Signal above threshold, processing..." << std::endl;
                    // Здесь можно обработать или отправить звук
                    // Обработка аудио-данных
                    PaError err = Pa_WriteStream(stream2, data.audioBuffer, FRAMES_PER_BUFFER);
                    if (err == paOutputUnderflowed) {
                        std::cerr << "PortAudio warning: Output underflowed." << std::endl;
                    } else {
                        handleError(err);
                    }// Формирование ответа клиенту
                } else {
                    std::cout << "Signal below threshold, ignoring..." << std::endl;
                    // Игнорируем звук, т.к. он слишком тихий
                }

            }
        std::lock_guard<std::mutex> lock(clientMutex);
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
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Создаем UDP сокет
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


    handleError(Pa_OpenDefaultStream(&stream2, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(stream2));

    std::cout << "Streaming audio to " << SERVER_IP << ":" << PORT << "..." << std::endl;


    std::strcpy(data.username, USERNAME);  // Копируем имя пользователя в объект
    std::strcpy(data.keyRoom, KEY_ROOM);  // Копируем ключ канала в объект
    data.type = type;  // Устанавливаем тип данных

    sockaddr_in clientAddr{};
    int clientAddrLen = sizeof(clientAddr);

    // запрос на подключение
    std::cout << "Waiting for connection..." << std::endl;
    DataMessage request{};
    std::strcpy(request.username, USERNAME);
    std::strcpy(request.keyRoom, KEY_ROOM);
    request.type = 2;
    int sendResult = sendto(sockfd, (char*)&request, sizeof(request), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sendResult == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }
    // Ожидание ответа от сервера
    char response[8];
    int bytesReceived = recvfrom(sockfd, response, sizeof(response), 0, (sockaddr*)&clientAddr, &clientAddrLen);
    if (bytesReceived > 0) {
        response[bytesReceived] = '\0'; // Завершаем строку нуль-терминатором
        if (strcmp(response, "yes") == 0) {
            std::cout << "Connected" << std::endl;
        }
        else {
            std::cout << response << std::endl;
            std::cout << "Connection failed" << std::endl;
            return -1;
        }
    } else {
        std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
    }
    roomThread = std::thread(&streamAudioThread);
    while (true) {
        // Чтение аудио данных из устройства
        handleError(Pa_ReadStream(stream, data.audioBuffer, FRAMES_PER_BUFFER));

        // Логируем отправку данных
        int sendResult = sendto(sockfd, (char*)&data, sizeof(data), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sendResult == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        } else {
            std::cout << "Sent data to server" << std::endl; // Логирование отправки
        }
        std::lock_guard<std::mutex> lock(clientMutex);
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
