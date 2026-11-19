cmake_minimum_required(VERSION 3.16)
project(get_info_client VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_executable(client
    src/main.c
    src/client.c
)

target_include_directories(client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS client RUNTIME DESTINATION bin)
