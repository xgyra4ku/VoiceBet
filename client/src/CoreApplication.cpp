#include "../inc/CoreApplication.h"
#include <iostream>
#include <winsock2.h>  // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <cstring>  // Для работы с C-строками
#include <thread>  // Для работы с многозадачностью (если понадобится)
#include <ws2tcpip.h>  // Для inet_pton, преобразование IP-адресов
#include <cmath>
#include <mutex>

CoreApplication::CoreApplication() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Инициализация библиотеки Winsock версии 2.2
    if (result != 0) {
        // Если инициализация не удалась
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return; // Выход из программы с ошибкой
    }

    // Создание UDP сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Создаем UDP сокет
    if (sockfd == INVALID_SOCKET) {
        // Если создание сокета не удалось
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup(); // Завершаем работу с Winsock
        return; // Выход с ошибкой
    }

    serverAddr.sin_family = AF_INET; // Используем IPv4
    serverAddr.sin_port = htons(PORT); // Устанавливаем порт (переводим в сетевой порядок байтов)
    clientAddrLen = sizeof(clientAddr);

    // Преобразуем IP-адрес в двоичный формат
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        // Проверка валидности IP-адреса
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        closesocket(sockfd); // Закрытие сокета
        WSACleanup(); // Очистка Winsock
        return ; // Выход с ошибкой
    }


    // Инициализация PortAudio
    handleError(Pa_Initialize());  // Инициализация библиотеки PortAudio
    handleError(Pa_OpenDefaultStream(&streamAudio, 0, 1, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(streamAudio));  // Запуск потока


    handleError(Pa_OpenDefaultStream(&recordAudio, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, nullptr, nullptr));  // Открытие аудио потока
    handleError(Pa_StartStream(recordAudio));
}

CoreApplication::~CoreApplication() {
    // Завершение работы
    handleError(Pa_StopStream(streamAudio));  // Остановка потока
    handleError(Pa_CloseStream(recordAudio));  // Закрытие потока
    Pa_Terminate();  // Завершение работы с PortAudio

    // Закрытие сокета и очистка Winsock
    closesocket(sockfd);
    WSACleanup();
}

void CoreApplication::handleError(PaError err) {
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

bool CoreApplication::isSignalAboveThreshold(const float *buffer, int frames) {

    float maxAmplitude = 0.0f;

    for (int i = 0; i < frames; ++i) {
        maxAmplitude = std::max(maxAmplitude, std::fabs(buffer[i]));
    }

    return maxAmplitude > THRESHOLD;  // Если максимальная амплитуда больше порога, то сигнал значимый
}

void CoreApplication::run() {
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
            return;
        }
    } else {
        std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
    }
    std::strcpy(request.username, USERNAME);  // Копируем имя пользователя в объект
    std::strcpy(request.keyRoom, KEY_ROOM);  // Копируем ключ канала в объект
    request.type = type;  // Устанавливаем тип данных
    room = new Room(KEY_ROOM, sockfd, request, serverAddr);
    room->start();
    while (true) {

        // Получение ответа от сервера
        int bytesReceived = recvfrom(sockfd, (char*)&request, sizeof(request), 0, (sockaddr*)&clientAddr, &clientAddrLen);
        if (bytesReceived > 0) {
            if (request.type == 1) {
                room->processMessage(request);
            } else if (request.type == 3) {

            }
        }
    }
}



