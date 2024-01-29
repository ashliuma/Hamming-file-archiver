add_library(HamEncoder hamming/hamming.h
        hamming/hamming.cpp
        hamming/encoder.h
        hamming/encoder.cpp
)

add_library(ArgParser parser/argparser.h parser/argparser.cpp)

add_library(Archiver archiver.h archiver.cpp)