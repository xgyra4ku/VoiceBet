cmake_minimum_required(VERSION 3.10)
project(VoiceChat)

# Установка пути для сторонних библиотек
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# Добавляем подпроекты
add_subdirectory(server)
add_subdirectory(client)
