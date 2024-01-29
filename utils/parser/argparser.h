#pragma once

#include <vector>
#include <string>
#include <cinttypes>
#include <cstring>


enum class Action {
    create, list, extract, append, del, concatenate
};

struct Commands {
    Action command;

    std::string arch_name_;
    std::vector<std::string> file_names_;
    uint8_t control_bits_count_;

    void Parse(int argc, char** argv);
};