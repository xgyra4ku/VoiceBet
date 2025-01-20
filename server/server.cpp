#include <iostream>  // Для вывода на экран
#include <cstring>   // Для работы с C-строками, например, memset
#include <thread>    // Для работы с многозадачностью (в случае использования потоков)
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

constexpr int SAMPLE_RATE = 44100;  // Частота дискретизации (частота аудио)
constexpr int FRAMES_PER_BUFFER = 1024;  // Количество сэмплов в одном аудио-буфере
constexpr int PORT = 12345;  // Порт для связи с клиентом

// Структура для хранения аудио данных
struct AudioData {
    float buffer[FRAMES_PER_BUFFER];  // Буфер для аудио данных
};

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
    if (sockfd < 0) {  // Если создание сокета не удалось
        perror("Socket creation failed");
        WSACleanup();  // Завершаем работу с Winsock
        return -1;  // Выход с ошибкой
    }

    sockaddr_in serverAddr{};  // Структура для хранения адреса сервера
    serverAddr.sin_family = AF_INET;  // Используем IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // Слушаем на всех интерфейсах
    serverAddr.sin_port = htons(PORT);  // Устанавливаем порт (переводим в сетевой порядок байтов)

    if (bind(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {  // Привязываем сокет к адресу
        perror("Bind failed");
        closesocket(sockfd);  // Закрываем сокет, если привязка не удалась
        WSACleanup();
        return -1;
    }

    // Инициализация PortAudio
    handleError(Pa_Initialize());  // Инициализация библиотеки PortAudio
    PaStream* stream;  // Поток для аудио
    handleError(Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(stream));  // Запуск потока

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;  // Информация о запуске сервера

    AudioData data{};  // Создаем объект для хранения аудио данных
    sockaddr_in clientAddr{};  // Адрес клиента
    int clientAddrLen = sizeof(clientAddr);

    while (true) {  // Бесконечный цикл для приема данных
        ssize_t bytesReceived = recvfrom(sockfd, (char*)&data, sizeof(data), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived > 0) {
            std::cout << "Received " << bytesReceived << " bytes" << std::endl; // Логирование
            handleError(Pa_WriteStream(stream, data.buffer, FRAMES_PER_BUFFER));
        } else {
            std::cerr << "No data received or error occurred" << std::endl;
        }

    }


    // Завершение работы
    handleError(Pa_StopStream(stream));  // Остановка потока
    handleError(Pa_CloseStream(stream));  // Закрытие потока
    Pa_Terminate();  // Завершение работы с PortAudio
    closesocket(sockfd);  // Закрытие сокета
    WSACleanup();  // Очистка Winsock
    return 0;
}
