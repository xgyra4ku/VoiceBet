cmake_minimum_required(VERSION 3.10)
project(VoiceBetServer)

set(CMAKE_CXX_STANDARD 17)

# Указываем путь к исходным файлам PortAudio
set(PORTAUDIO_INCLUDE_DIR E:/C_Projects_VS/cmake_project/voicebetServer/third_party/portaudio/include)

# Указываем путь к библиотекам PortAudio
set(PORTAUDIO_LIB_DIR E:/C_Projects_VS/cmake_project/voicebetServer/third_party/portaudio/build)

# Добавляем путь к заголовочным файлам PortAudio
include_directories(${PORTAUDIO_INCLUDE_DIR})

# Добавляем библиотеки для линковки
link_directories(${PORTAUDIO_LIB_DIR})

# Добавляем основной исходный файл для сервера
add_executable(Server main.cpp
        src/ClientHandler.cpp
        src/Server.cpp
        inc/ClientHandler.h
        inc/Server.h
        main.cpp)

# Линкуем библиотеку PortAudio
target_link_libraries(Server portaudio)

# Линкуем библиотеку ws2_32 для работы с сокетами
target_link_libraries(Server ws2_32)
