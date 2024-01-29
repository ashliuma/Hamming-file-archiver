cmake_minimum_required(VERSION 3.12)
project(labwork6
        VERSION 1.0
        DESCRIPTION "C++ Year 2023 labwork-6"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(labwork6
        bin/main.cpp
        utils/parser/argparser.h
        utils/parser/argparser.cpp
        utils/hamming/hamming.h
        utils/hamming/hamming.cpp
        utils/hamming/encoder.h
        utils/hamming/encoder.cpp
        utils/archiver.h
        utils/archiver.cpp)
