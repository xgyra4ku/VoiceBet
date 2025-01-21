#include "../inc/Server.h"
#include <iostream>     // Для вывода на экран
#include <cstring>      // Для работы с C-строками, например, memset
#include <thread>       // Для работы с многозадачностью (в случае использования потоков)
#include <winsock2.h>   // Для работы с сокетами на Windows
#include <portaudio.h>  // Для работы с аудио
#include <map>          // для работы с контейнерами

Server::Server() {
    // Инициализация Winsock
    WSADATA wsaData;
    result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Инициализация библиотеки Winsock версии 2.2
    if (result != 0) {
        // Если инициализация не удалась
        std::cerr << "WSAStartup failed: " << result << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Создание UDP сокета
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // Создаем UDP сокет
    if (sockfd < 0) {
        // Если создание сокета не удалось
        perror("Socket creation failed");
        WSACleanup(); // Завершаем работу с Winsock
        std::exit(EXIT_FAILURE);
    }
    serverAddr.sin_family = AF_INET; // Используем IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Слушаем на всех интерфейсах
    serverAddr.sin_port = htons(PORT); // Устанавливаем порт (переводим в сетевой порядок байтов)

    if (bind(sockfd, (sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        // Привязываем сокет к адресу
        perror("Bind failed");
        closesocket(sockfd); // Закрываем сокет, если привязка не удалась
        WSACleanup();
        std::exit(EXIT_FAILURE);
    }
}

Server::~Server() = default;

void Server::run() {
    std::cout << "Server is listening on port " << PORT << "..." << std::endl;
    while (true) {
        sockaddr_in clientAddr{};  // Локальная переменная
        int clientAddrLen = sizeof(clientAddr);

        ssize_t bytesReceived = recvfrom(sockfd, (char*)&dataPackage, sizeof(dataPackage), 0,
                                         (sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived > 0) {
            if (dataPackage.type == 1) {
                rooms[dataPackage.keyRoom]->processMessage(dataPackage, clientAddr);
            }
            else if (dataPackage.type == 2) {
                const char* response = "yes";
                ssize_t bytesSent = sendto(sockfd, response, strlen(response), 0,
                                           (sockaddr*)&clientAddr, clientAddrLen);
                if (bytesSent > 0) {
                    std::cout << "Client: " << dataPackage.username
                              << " joined the room: " << dataPackage.keyRoom << std::endl;

                    // Создаём новую комнату, если она ещё не существует
                    if (rooms.find(dataPackage.keyRoom) == rooms.end()) {
                        rooms[dataPackage.keyRoom] = new RoomHandler(dataPackage.keyRoom, sockfd);
                        rooms[dataPackage.keyRoom]->start();
                    }

                    // Добавляем клиента в комнату
                    rooms[dataPackage.keyRoom]->addClient(clientAddr);
                } else {
                    perror("Failed to connect to client");
                }
            }
        }
    }

    closesocket(sockfd);
    WSACleanup();
}

void Server::handleError(const PaError err) {
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
