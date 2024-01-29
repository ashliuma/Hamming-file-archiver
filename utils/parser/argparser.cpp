#include "argparser.h"


void Commands::Parse(int argc, char** argv) {
    bool is_arch_name = false;
    bool is_bits = false;

    for (size_t i = 1; i < argc; i++) {
        if (is_arch_name) {
            is_arch_name = false;
            arch_name_ = argv[i];
        } else if (is_bits) {
            is_bits = false;
            control_bits_count_ = atoi(argv[i]);
        } else if (argv[i][0] == '-') {

            if ((strcmp(argv[i], "--create") == 0) || (strcmp(argv[i], "-c") == 0)) {
                command = Action::create;

            } else if ((strcmp(argv[i], "--list") == 0) || (strcmp(argv[i], "-l") == 0)) {
                command = Action::list;

            } else if ((strcmp(argv[i], "--extract") == 0) || (strcmp(argv[i], "-x") == 0)) {
                command = Action::extract;

            } else if ((strcmp(argv[i], "--append") == 0) || (strcmp(argv[i], "-a") == 0)) {
                command = Action::append;

            } else if ((strcmp(argv[i], "--delete") == 0) || (strcmp(argv[i], "-d") == 0)) {
                command = Action::del;

            } else if ((strcmp(argv[i], "--concatenate") == 0) || (strcmp(argv[i], "-A") == 0)) {
                command = Action::concatenate;

            } else if ((strcmp(argv[i], "--file") == 0) || (strcmp(argv[i], "-f") == 0)) {
                is_arch_name = true;

            } else if ((strcmp(argv[i], "-bits") == 0) || (strcmp(argv[i], "-b") == 0)) {
                is_bits = true;

            } else {
                std::string argument = argv[i];
                std::string new_argument = argument.substr(0, 7);
                if (new_argument == "--file=") {
                    arch_name_ = argument.substr(7, argument.size() - 7);
                } else if (new_argument == "--bits=") {
                    control_bits_count_ = atoi(argument.substr(7, argument.size() - 7).c_str());
                }
            }
        } else {
            file_names_.emplace_back(argv[i]);
        }
    }
}
