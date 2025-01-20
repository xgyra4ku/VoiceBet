cmake_minimum_required(VERSION 3.10)
project(VoiceBetClient)

set(CMAKE_CXX_STANDARD 17)

# Указываем путь к исходным файлам PortAudio
set(PORTAUDIO_INCLUDE_DIR E:/C_Projects_VS/cmake_project/voicebetServer/third_party/portaudio/include)

# Указываем путь к библиотекам PortAudio
set(PORTAUDIO_LIB_DIR E:/C_Projects_VS/cmake_project/voicebetServer/third_party/portaudio/build)

# Добавляем путь к заголовочным файлам PortAudio
include_directories(${PORTAUDIO_INCLUDE_DIR})

# Добавляем библиотеки для линковки
link_directories(${PORTAUDIO_LIB_DIR})

# Указываем, какие библиотеки линковать с проектом
add_executable(Client client.cpp)

# Линкуем библиотеку PortAudio
target_link_libraries(Client portaudio)

# Добавляем библиотеку ws2_32 для работы с сокетами
target_link_libraries(Client ws2_32)
