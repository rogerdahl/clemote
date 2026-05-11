cmake_minimum_required(VERSION 3.9)
project(clemote)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS -Wfatal-errors)


link_libraries(
    fmt
    sdbus-c++
    tag
    pthread
    asound
)

add_executable(
    clemote
    src/main.cpp
    src/clementine_dbus.cpp
    src/tag.cpp
    src/alsa_volume.cpp
    src/event_code_to_str.cpp
    src/pulse_audio.cpp
)

