cmake_minimum_required(VERSION 3.20)

# ESPP utilise C++23 ; le projet reste un projet ESP-IDF natif.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(roza_firmware)
