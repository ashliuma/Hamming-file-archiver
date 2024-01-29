#include "../utils/parser/argparser.h"
#include "../utils/archiver.h"
#include "../utils/hamming/hamming.h"
#include "../utils/hamming/encoder.h"


int main(int argc, char** argv) {

    Commands args;
    args.Parse(argc, argv);

    Archiver::Archive(commands);

    return 0;
}
