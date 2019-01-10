cmake_minimum_required(VERSION 3.9)
project(clemote)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(
  Boost COMPONENTS system filesystem REQUIRED
)

include_directories(
  libraries/sdbus-cpp/include
)

link_directories(
  libraries/sdbus-cpp/src/.libs
)

link_libraries(
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  evdev
  fmt
  pulse
  sdbus-c++
  systemd
  tag
  z
)

add_executable(
  clemote
  src/main.cpp
  src/clementine_dbus.cpp
  src/tag.cpp
  src/pulse_audio.cpp
)
