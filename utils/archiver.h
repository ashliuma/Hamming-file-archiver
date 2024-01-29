#pragma once

#include "parser/argparser.h"
#include "hamming/hamming.h"
#include "hamming/encoder.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstdio>

namespace Archiver {
    struct Archive {
        std::string arch_name;

        Archive();

        void Clear();

        void AddFileToArchive(const std::string& file_name, uint8_t control_bits_count);

        uint8_t ExtractFileFromArchive(const std::string& file_name, const std::string& path);

        std::vector<std::string> FileList();

        void DeleteFileFromArchive(const std::string& file_name);

        void AddFilesFromArchive(const std::string& arch);
    };

    static const uint16_t kCountHeaderSize = 8;
    static const uint16_t kEncodedCountHeaderSize = kCountHeaderSize * 2;
    static const uint16_t kNameHeaderSize = 25;
    static const uint16_t kEncodedNameHeaderSize = kNameHeaderSize * 2;
    constexpr static const uint16_t kBits[] = {0, 4};
    static const std::string kArchExt = "arch.haf";
    static bool isCorrectlyDecoded = true;
    
    void Create(const std::string& arch_name, const std::vector<std::string>& file_names, uint8_t control_bits_count);

    void List(const std::string& arch_name);

    void Extract(const std::string& arch_name, const std::vector<std::string>& file_names);

    void Append(const std::string& arch_name, const std::vector<std::string>& file_names, uint8_t control_bits_count);

    void Delete(const std::string& arch_name, const std::vector<std::string>& file_names);

    void Concatenate(const std::string& arch_name, const std::vector<std::string>& file_names);

    static void CountHeaderFill(uint8_t *count_header, uint64_t bytes_count_encoded);
    
    void AddEncodedByteToArchive(uint8_t current_byte, std::ofstream &arch);
};

