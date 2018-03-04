# Set in CLion CMake Environment:
# export CC=/usr/local/gcc-7.3/bin/gcc-7.3
# export CXX=/usr/local/gcc-7.3/bin/g++-7.3
# export LD_LIBRARY_PATH=/usr/local/gcc-7.3/lib64/

cmake_minimum_required(VERSION 3.9)
project(clemote)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(
  libraries/fmt-3.0.1
  libraries/taglib/mytaglib/include
  libraries/sdbus-cpp/include
)

link_directories(
  /usr/local/gcc-7.3/lib64/
  libraries/fmt-3.0.1/fmt
  libraries/taglib/mytaglib/lib
  libraries/sdbus-cpp/src/.libs
)

link_libraries(
  boost_filesystem
  boost_system
  evdev
  fmt
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
)
