#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <cstdio>

#include "hamming.h"


namespace HamArchiver{

    const uint16_t kCountHeaderSize = 8;
    const uint16_t kEncodedCountHeaderSize = kCountHeaderSize * 2;
    const uint16_t kNameHeaderSize = 25;
    const uint16_t kEncodedNameHeaderSize = kNameHeaderSize * 2;
    const uint16_t kBits[] = {0, 4};
    const std::string arch_tmp_name = "arch.tmp";

    struct File {

        std::string name_;
        uint64_t bytes_count_ = 0;
        uint8_t control_bits_ = 3; // Hamming bits count

        File();

        ~File();
        
        File(const std::string& file_name);

        File(const std::string& file_name, uint8_t ham_bits);
    };

    uint64_t BytesCount(std::ifstream& file);
    
    void AddEncodedToFile(const File& file, std::ofstream& output);

    struct HamNode {
        bool is_leaf;
        HamNode* left = nullptr; // zero edge
        HamNode* right = nullptr; // one edge
        
        uint64_t freq_ = 0;
        uint8_t value_;

        HamNode();

        HamNode(uint8_t value);

        HamNode(uint8_t value, uint64_t freq_);

        HamNode(HamNode* left, HamNode* right);
    };

    uint64_t Power(uint64_t left, uint8_t right);

    void AddEncodedByteToArchive(uint8_t current_byte, std::ofstream &arch);

    void AddDecodedHammingToFile(const File &file, std::ofstream &output);

    void AddCountHeaderToArchive(std::ofstream &arch, uint64_t bytes_count);
}
