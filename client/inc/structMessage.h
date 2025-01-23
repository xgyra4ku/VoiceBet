#ifndef STRUCTMESSAGE_H
#define STRUCTMESSAGE_H
constexpr int FRAMES_PER_BUFFER = 1024;  // Количество сэмплов в одном аудио-буфере

struct DataMessage {
    char username[128];                       // имя пользователя
    char keyRoom[256];                       // ключ канала
    int type;                              // 1 - аудио включено(голос), 0 - выключено(сообщение) 2 запрос на подключение
    float audioBuffer[FRAMES_PER_BUFFER];  // Буфер для аудио данных
    uint64_t message_id;
    bool is_ack;
};
#endif //STRUCTMESSAGE_H
