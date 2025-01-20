#include <iostream>  // Для вывода на экран
#include <cstring>   // Для работы с C-строками, например, memset
#include <thread>    // Для работы с многозадачностью (в случае использования потоков)
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <map>

#pragma comment(lib, "ws2_32.lib")  // Линковка библиотеки Winsock

constexpr int SAMPLE_RATE = 44100;  // Частота дискретизации (частота аудио)
constexpr int FRAMES_PER_BUFFER = 1024;  // Количество сэмплов в одном аудио-буфере
constexpr int PORT = 12345;  // Порт для связи с клиентом
std::map<std::string, std::string> rooms;

// Структура для хранения данных
struct DataMessage {
    char username[128];                   // имя пользователя
    char keyRoom[256];               // ключ канала
    char type;                         // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 - запрос на подключение
    float audioBuffer[FRAMES_PER_BUFFER];   // Буфер для аудио данных
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

    sockaddr_in serverAddr{}, clientAddr{};  // Структура для хранения адреса сервера
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

    DataMessage dataPackage{};  // Создаем объект для хранения аудио данных
    int clientAddrLen = sizeof(clientAddr);

    while (true) {  // Бесконечный цикл для приема данных
        // Получаем данные от клиента через сокет
        ssize_t bytesReceived = recvfrom(sockfd, (char*)&dataPackage, sizeof(dataPackage), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        // Проверяем, были ли получены данные
        if (bytesReceived > 0) {
            // Обработка данных от клиента

            if (dataPackage.type == 1) {
                // Обработка аудио-данных
               // PaError err = Pa_WriteStream(stream, dataPackage.audioBuffer, FRAMES_PER_BUFFER);
               // if (err == paOutputUnderflowed) {
                //    std::cerr << "PortAudio warning: Output underflowed." << std::endl;
               // } else {
                //    handleError(err);
                //}// Формирование ответа клиенту
                const char *response = "Data received successfully!";
                ssize_t bytesSent = sendto(sockfd, response, strlen(response), 0, (sockaddr *) &clientAddr,
                                           clientAddrLen);
                if (bytesSent > 0) {
                    std::cout << "Response sent to client." << std::endl;
                } else {
                    perror("Failed to send response");
                }
            }
            if (dataPackage.type == 2) {
                const char *response = "yes";
                ssize_t bytesSent = sendto(sockfd, response, strlen(response), 0, (sockaddr *) &clientAddr,clientAddrLen);
                if (bytesSent > 0) {
                    std::cout << "Client: " << dataPackage.username << " joined the room: " << dataPackage.keyRoom << std::endl;
                    rooms[dataPackage.keyRoom] = dataPackage.username;  // Добавляем клиента в комнату[]
                } else {
                    perror("Failed to connect to client");
                }
            }
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
