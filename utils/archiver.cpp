#include "archiver.h"

void Archivation(const Commands& commands) {
    switch (commands.command) {
        case Action::create:
            Archiver::Create(commands.arch_name_, commands.file_names_, commands.control_bits_count_);
            break;
        case Action::list:
            Archiver::List(commands.arch_name_);
            break;
        case Action::extract:
            Archiver::Extract(commands.arch_name_, commands.file_names_);
            break;
        case Action::append:
            Archiver::Append(commands.arch_name_, commands.file_names_, commands.control_bits_count_);
            break;
        case Action::del:
            Archiver::Delete(commands.arch_name_, commands.file_names_);
            break;
        case Action::concatenate:
            Archiver::Concatenate(commands.arch_name_, commands.file_names_);
            break;
    }
}

std::string NameWithoutExtension(const std::string& file_name) {
    for (size_t i = 0; i < file_name.size(); i++) {
        if (file_name[i] == '.') {
            return file_name.substr(0, i);
        }
    }
    return file_name;
}

void Archiver::CountHeaderFill(uint8_t count_header[], uint64_t bytes_count_encoded) {
    for (size_t i = 0; i < Archiver::kCountHeaderSize; i++) {
        count_header[i] = (bytes_count_encoded >> ((kCountHeaderSize - i - 1) * CHAR_BIT));
    }
}

void Archiver::AddEncodedByteToArchive(uint8_t current_byte, std::ofstream& arch) {
    bool current_bits[CHAR_BIT];
    size_t control_bit;
    std::vector<bool> hamming_code(CHAR_BIT, false);

    for (size_t j = 0; j < CHAR_BIT; j++) {
        current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
    }

    for (uint16_t bits_count: kBits) {
        control_bit = 1;
        for (size_t i = 0; i < CHAR_BIT; i++) {
            if (control_bit - 1 == i) {
                control_bit *= 2;
            } else {
                hamming_code[i] = current_bits[bits_count];
                bits_count++;
            }
        }
        HamEncoder::EncodeHamming(hamming_code);
        current_byte = 0;
        for (int i = 0; i < CHAR_BIT; i++) {
            if (hamming_code[i]) {
                current_byte += static_cast<uint8_t>(HamArchiver::Power(2, CHAR_BIT - i - 1));
            }
        }
        arch << current_byte;
    }
}

std::string FileName(const std::string& path) {
    for (int i = path.size() - 1; i >= 0; i--) {
        if (path[i] == '\\') {
            return path.substr(i + 1, path.size() - i - 1);
        }
    }
    return path;
}

Archiver::Archive ArchiveOpen(const std::string& arch_name) {
    Archiver::Archive archive;
    archive.arch_name = NameWithoutExtension(arch_name) + ".haf";
    return archive;
}

uint64_t BytesCount(std::ifstream& file) {
    char current;
    size_t bytes = 0;
    while (file.get(current)) {
        bytes++;
    }
    file.clear();
    file.seekg(0, std::ios::beg);
    return bytes;
}

void HamArchiver::AddCountHeaderToArchive(std::ofstream& arch, uint64_t bytes_count) {
    uint8_t current_byte;
    uint64_t bytes_count_encoded = bytes_count;
    uint8_t count_header[kCountHeaderSize];
    Archiver::CountHeaderFill(count_header, bytes_count_encoded);
    for (uint8_t k = 0; k < kCountHeaderSize; k++) {
        current_byte = count_header[k];
        AddEncodedByteToArchive(current_byte, arch);
    }
}

void AddNameHeaderToArchive(std::ofstream& arch, const std::string& path) {
    uint8_t current_byte;
    std::string file_name = FileName(path);
    uint8_t name[HamArchiver::kNameHeaderSize];
    for (size_t i = 0; i < HamArchiver::kNameHeaderSize; i++) {
        if (i < file_name.size()) {
            name[i] = file_name[i];
        } else {
            name[i] = '\0';
        }
    }
    for (uint64_t k = 0; k < HamArchiver::kNameHeaderSize; k++) {
        current_byte = name[k];
        HamArchiver::AddEncodedByteToArchive(current_byte, arch);
    }
}

void AddBitsHeaderToArchive(std::ofstream& arch, uint8_t control_bits_count) {
    HamArchiver::AddEncodedByteToArchive(control_bits_count, arch);
}

void AddEncodedFileToArchive(std::ifstream& file, std::ofstream& arch, uint8_t control_bits_count) {
    uint8_t current_byte;
    uint64_t bytes_count = BytesCount(file);

    if (control_bits_count == 3) {
        for (size_t i = 0; i < bytes_count; i++) {
            current_byte = file.get();
            HamArchiver::AddEncodedByteToArchive(current_byte, arch);
        }
        
    } else if (control_bits_count >= 4) {
        uint64_t hamming_bits_count = HamArchiver::Power(2, control_bits_count);
        uint64_t info_bits_count = hamming_bits_count - control_bits_count - 1;
        
        std::vector<bool> cur_hamming_bits(hamming_bits_count, false);
        std::vector<bool> next_hamming_bits(CHAR_BIT, false);
        
        uint64_t prev_bits_count = 0;
        uint64_t control_bit = 1;
        uint64_t current_bit = 0;
        uint64_t next_bits_count = 0;
        bool current_bits[CHAR_BIT];
        uint64_t cur_bytes_count = 0;
        uint64_t bytes_to_add = 0;

        while ((cur_bytes_count < bytes_count) || (prev_bits_count != 0)) {
            bytes_to_add =
                    (info_bits_count - prev_bits_count) / 8 +
                    ((info_bits_count - prev_bits_count) % 8 != 0);
            for (size_t i = 0; (i < bytes_to_add) && (cur_bytes_count < bytes_count); i++) {
                cur_bytes_count++;
                current_byte = file.get();

                for (size_t j = 0; j < CHAR_BIT; j++) {
                    current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
                }

                for (size_t j = 0; j < CHAR_BIT; j++) {
                    while (current_bit == control_bit - 1) {
                        current_bit++;
                        control_bit *= 2;
                    }
                    if (current_bit >= hamming_bits_count) {
                        next_hamming_bits[next_bits_count++] = current_bits[j];
                    } else {
                        cur_hamming_bits[current_bit++] = current_bits[j];
                    }
                }
            }

            HamEncoder::EncodeHamming(cur_hamming_bits);

            for (uint64_t i = 0; i < hamming_bits_count; i += 8) {
                current_byte = 0;
                for (size_t j = 0; j < CHAR_BIT; j++) {
                    if (cur_hamming_bits[i + j]) {
                        current_byte += static_cast<uint8_t>(HamArchiver::Power(2, CHAR_BIT - j - 1));
                    }
                }
                arch << current_byte;

            }

            for (uint64_t i = 0; i < hamming_bits_count; i++) {
                cur_hamming_bits[i] = false;
            }

            current_bit = 0;
            control_bit = 1;
            for (int i = 0; i < next_bits_count; i++) {
                while (current_bit == control_bit - 1) {
                    current_bit++;
                    control_bit *= 2;
                }
                cur_hamming_bits[current_bit++] = next_hamming_bits[i];
            }
            prev_bits_count = next_bits_count;
            next_bits_count = 0;
        }
    }
}


void Archiver::Archive::AddFileToArchive(const std::string& file_name, uint8_t control_bits_count) {
    std::ifstream file;
    std::ofstream arch;
    file.open(file_name, std::ios::binary);
    arch.open(arch_name, std::ios::app | std::ios::binary);

    uint64_t bytes_count = BytesCount(file);
    
    HamArchiver::AddCountHeaderToArchive(arch, bytes_count);

    AddNameHeaderToArchive(arch, file_name);

    AddBitsHeaderToArchive(arch, control_bits_count);

    AddEncodedFileToArchive(file, arch, control_bits_count);

    arch.close();
    file.close();
}

uint8_t DecodeHammingToByte(uint8_t first_byte, uint8_t second_byte) {
    uint8_t result_byte = 0;
    std::vector<bool> hamming_code1(CHAR_BIT);
    std::vector<bool> hamming_code2(CHAR_BIT);
    std::vector<bool> hamming_decoded(CHAR_BIT);
    for (size_t j = 0; j < CHAR_BIT; j++) {
        hamming_code1[CHAR_BIT - j - 1] = (first_byte >> j) & 1;
    }
    for (size_t j = 0; j < CHAR_BIT; j++) {
        hamming_code2[CHAR_BIT - j - 1] = (second_byte >> j) & 1;
    }

    bool isFirstPartDecoded =HamEncoder::DecodeHamming(hamming_code1);
    bool isSecondPartDecoded = HamEncoder::DecodeHamming(hamming_code2);

    if (!isFirstPartDecoded || !isSecondPartDecoded) {
        Archiver::isCorrectlyDecoded = false;
    }

    hamming_decoded[0] = hamming_code1[2];
    hamming_decoded[1] = hamming_code1[4];
    hamming_decoded[2] = hamming_code1[5];
    hamming_decoded[3] = hamming_code1[6];

    hamming_decoded[4] = hamming_code2[2];
    hamming_decoded[5] = hamming_code2[4];
    hamming_decoded[6] = hamming_code2[5];
    hamming_decoded[7] = hamming_code2[6];

    for (size_t j = 0; j < CHAR_BIT; j++) {
        if (hamming_decoded[j]) {
            result_byte += HamArchiver::Power(2, CHAR_BIT - j - 1);
        }
    }

    return result_byte;
}

uint64_t DecodeCountHeader(uint8_t current_byte, std::ifstream& arch) {
    uint64_t bytes_count = 0;
    uint8_t count_header_encoded[Archiver::kEncodedCountHeaderSize];
    count_header_encoded[0] = current_byte;
    uint8_t count_header[Archiver::kCountHeaderSize];

    for (size_t i = 1; i < Archiver::kEncodedCountHeaderSize; i++) {
        arch >> current_byte;
        count_header_encoded[i] = current_byte;
    }

    for (size_t i = 0; i < Archiver::kCountHeaderSize; i++) {
        count_header[i] = DecodeHammingToByte(count_header_encoded[i * 2], count_header_encoded[i * 2 + 1]);
    }

    for (size_t i = 0; i < Archiver::kCountHeaderSize; i++) {
        bytes_count += (count_header[i] << ((Archiver::kCountHeaderSize - i - 1) * CHAR_BIT));
    }

    return bytes_count;
}

std::string DecodeNameHeader(std::ifstream& arch) {
    uint8_t current_byte;
    uint8_t name_header_encoded[Archiver::kEncodedNameHeaderSize];
    uint8_t name_header[Archiver::kNameHeaderSize];

    for (size_t i = 0; i < Archiver::kEncodedNameHeaderSize; i++) {
        arch >> current_byte;
        name_header_encoded[i] = current_byte;
    }

    for (size_t i = 0; i < Archiver::kNameHeaderSize; i++) {
        name_header[i] = DecodeHammingToByte(name_header_encoded[i * 2], name_header_encoded[i * 2 + 1]);
    }
    std::string file_name(reinterpret_cast<const char* const>(name_header));

    return file_name;
}

uint8_t DecodeBitsHeader(std::ifstream& arch) {
    uint8_t first_byte;
    uint8_t second_byte;
    arch >> first_byte;
    arch >> second_byte;

    uint8_t result = DecodeHammingToByte(first_byte, second_byte);

    return result;
}

uint8_t Archiver::Archive::ExtractFileFromArchive(const std::string& file_name, const std::string& path) {
    std::ofstream file;
    std::ifstream arch;
    arch.open(arch_name, std::ios::in | std::ios::binary);
    file.open(path + '\\' + file_name, std::ios::binary);

    uint8_t current_byte;
    uint64_t bytes_count;
    std::string current_file_name;
    uint8_t control_bits_count;


    while (arch >> current_byte) {
        bytes_count = DecodeCountHeader(current_byte, arch);
        uint64_t bits_count = bytes_count * 8;

        current_file_name = DecodeNameHeader(arch);

        control_bits_count = DecodeBitsHeader(arch);

        uint64_t hamming_bits_count = HamArchiver::Power(2, control_bits_count);
        uint64_t hamming_bytes_count = hamming_bits_count / CHAR_BIT;
        uint64_t info_bits_count = hamming_bits_count - control_bits_count - 1;
        uint64_t encoded_bytes_count =((bytes_count * CHAR_BIT / info_bits_count) +
                (bytes_count * 8 % info_bits_count != 0)) * hamming_bytes_count;

        if (current_file_name == file_name) {
            if (control_bits_count == 3) {
                uint8_t current_byte1;
                uint8_t current_byte2;
                uint8_t decoded_byte;

                for (size_t i = 0; i < encoded_bytes_count; i+=2) {
                    arch >> current_byte1;
                    arch >> current_byte2;
                    decoded_byte = DecodeHammingToByte(current_byte1, current_byte2);
                    file << decoded_byte;
                }

            } else if (control_bits_count >= 4) {
                std::vector<bool> cur_hamming_bits(hamming_bits_count, false);
                std::vector<bool> to_byte(CHAR_BIT);
                uint64_t cur_bytes_count = 0;
                bool current_bits[CHAR_BIT];
                bool byte[CHAR_BIT];
                uint64_t current_bit = 0;
                uint64_t control_bit = 1;
                uint64_t current_to_byte_bit = 0;


                for (size_t i = 0; i < encoded_bytes_count; i += hamming_bytes_count) {
                    current_bit = 0;
                    control_bit = 1;
                    for (size_t j = 0; j < hamming_bytes_count; j++) {

                        current_byte = arch.get();

                        for (size_t k = 0; k < CHAR_BIT; k++) {
                            byte[CHAR_BIT - k - 1] = (current_byte >> k) & 1;
                        }

                        for (size_t k = 0; k < CHAR_BIT; k++) {
                            cur_hamming_bits[current_bit++] = byte[k];
                        }
                    }

                    bool byte_decode = HamEncoder::DecodeHamming(cur_hamming_bits);

                    if (!byte_decode && isCorrectlyDecoded) {
                        isCorrectlyDecoded = false;
                    }

                    control_bit = 1;
                    current_byte = 0;

                    for (uint64_t k = 0; k < hamming_bits_count; k++) {
                        if (cur_bytes_count == bytes_count) {
                            break;
                        }
                        if (k == control_bit - 1) {
                            control_bit *= 2;
                            continue;
                        }
                        current_bits[current_to_byte_bit++] = cur_hamming_bits[k];
                        if (current_to_byte_bit == 8) {
                            current_to_byte_bit = 0;
                            for (size_t j = 0; j < CHAR_BIT; j++) {
                                if (current_bits[j]) {
                                    current_byte += static_cast<uint8_t>(HamArchiver::Power(2, CHAR_BIT - j - 1));
                                }
                            }
                            file << current_byte;
                            cur_bytes_count++;
                            current_byte = 0;
                        }
                    }
                }
            }
            return control_bits_count;
        } else {
            arch.seekg(encoded_bytes_count, std::ios::cur);
        }
    }
    std::cerr << "Can't find file in archive.";
    return 0;
}

std::vector<std::string> Archiver::Archive::FileList() {

    std::vector<std::string> file_list;

    std::ifstream arch(arch_name, std::ios::binary | std::ios::in);

    uint8_t current_byte;
    uint64_t bytes_count;
    std::string current_file_name;
    uint8_t control_bits_count;

    while (arch >> current_byte) {

        bytes_count = DecodeCountHeader(current_byte, arch);

        current_file_name = DecodeNameHeader(arch);

        control_bits_count = DecodeBitsHeader(arch);

        file_list.push_back(current_file_name);

        uint64_t hamming_bits_count = HamArchiver::Power(2, control_bits_count);
        uint64_t hamming_bytes_count = hamming_bits_count / CHAR_BIT;
        uint64_t info_bits_count = hamming_bits_count - control_bits_count - 1;
        uint64_t encoded_bytes_count =
                ((bytes_count * CHAR_BIT / info_bits_count) + (bytes_count * 8 % info_bits_count != 0)) *
                hamming_bytes_count;

        arch.seekg(encoded_bytes_count, std::ios::cur);
    }
    return file_list;
}

void Archiver::Archive::DeleteFileFromArchive(const std::string& file_name) {

    Archive arch_tmp = ArchiveOpen(kArchExt);

    std::vector<std::string> file_list = FileList();

    uint8_t control_bits_count;

    for (size_t i = 0; i < file_list.size(); i++) {
        if (file_list[i] != file_name) {
            control_bits_count = ExtractFileFromArchive(file_list[i], std::filesystem::temp_directory_path().string());
            arch_tmp.AddFileToArchive(std::filesystem::temp_directory_path().string() + file_list[i],
                                      control_bits_count);
        }
    }
    remove(arch_name.c_str());
    rename(kArchExt.c_str(), arch_name.c_str());
}

Archiver::Archive::Archive() {
    arch_name = kArchExt;
}


void Archiver::Archive::Clear() {
    std::ofstream arch;
    arch.open(arch_name, std::ios::trunc | std::ios::binary);
}

void Archiver::Archive::AddFilesFromArchive(const std::string& arch) {

    Archive arch_from = ArchiveOpen(arch);

    std::vector<std::string> file_list = arch_from.FileList();

    uint8_t control_bits_count;

    for (size_t i = 0; i < file_list.size(); i++) {
        control_bits_count = arch_from.ExtractFileFromArchive(file_list[i],
                                                              std::filesystem::temp_directory_path().string());
        AddFileToArchive(std::filesystem::temp_directory_path().string() + file_list[i], control_bits_count);
    }
}

void CheckErrors() {
    if (!Archiver::isCorrectlyDecoded) {
        std::cerr << "Archived files can't be decoded correctly!";
    }
}

void Archiver::Create(const std::string& arch_name, const std::vector<std::string>& file_names, uint8_t control_bits_count) {
    Archiver::Archive archive = ArchiveOpen(arch_name);
    archive.Clear();
    for (size_t i = 0; i < file_names.size(); i++) {
        archive.AddFileToArchive(file_names[i], control_bits_count);
    }
    CheckErrors();
}

void Archiver::List(const std::string& arch_name) {
    Archiver::Archive archive = ArchiveOpen(arch_name);
    std::vector<std::string> file_names = archive.FileList();
    for (size_t i = 0; i < file_names.size(); i++) {
        std::cout << file_names[i] << '\n';
    }
    CheckErrors();
}

void Archiver::Extract(const std::string& arch_name, const std::vector<std::string>& file_names) {
    Archiver::Archive archive = ArchiveOpen(arch_name);
    std::vector<std::string> files_extract;
    if (file_names.empty()) {
        files_extract = archive.FileList();
    } else {
        files_extract = file_names;
    }
    std::filesystem::create_directory(NameWithoutExtension(arch_name));
    for (size_t i = 0; i < files_extract.size(); i++) {
        archive.ExtractFileFromArchive(files_extract[i], NameWithoutExtension(arch_name));
    }
    CheckErrors();
}

void Archiver::Append(const std::string& arch_name, const std::vector<std::string>& file_names, uint8_t control_bits_count) {
    Archive archive = ArchiveOpen(arch_name);
    for (size_t i = 0; i < file_names.size(); i++) {
        archive.AddFileToArchive(file_names[i], control_bits_count);
    }
    CheckErrors();
}

void Archiver::Delete(const std::string& arch_name, const std::vector<std::string>& file_names) {
    Archive archive = ArchiveOpen(arch_name);
    for (size_t i = 0; i < file_names.size(); i++) {
        archive.DeleteFileFromArchive(file_names[i]);
    }
    CheckErrors();
}

void Archiver::Concatenate(const std::string& arch_name, const std::vector<std::string>& file_names) {
    Archive archive = ArchiveOpen(arch_name);
    archive.Clear();
    for (size_t i = 0; i < file_names.size(); i++) {
        archive.AddFilesFromArchive(file_names[i]);
    }
    CheckErrors();
}
