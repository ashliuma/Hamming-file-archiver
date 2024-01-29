#include "encoder.h"

HamArchiver::File::File(){

};

HamArchiver::File::~File(){

};

HamArchiver::File::File(const std::string& file_name): name_(file_name) {
    std::ifstream file;
    file.open(file_name);
    bytes_count_ = BytesCount(file);
}

HamArchiver::File::File(const std::string& file_name, uint8_t ham_bits) : File(file_name) {
    this->control_bits_ = ham_bits;
}

uint64_t HamArchiver::BytesCount(std::ifstream &file) {
    char current;
    size_t bytes = 0;
    while (file.get(current)) {
        bytes++;
    }
    file.clear();
    file.seekg(0, std::ios::beg);
    return bytes;
}

HamArchiver::HamNode::HamNode() {

}

HamArchiver::HamNode::HamNode(uint8_t value): value_(value){
        is_leaf = true;
}

HamArchiver::HamNode::HamNode(uint8_t value, uint64_t freq): value_(value), freq_(freq){
    is_leaf = true;
}

HamArchiver::HamNode::HamNode(HamNode* left, HamNode* right): left(left), right(right){
    is_leaf = false;
    this->freq_ = left->freq_ + right->freq_;
}

bool operator<(const HamArchiver::HamNode& lhs, const HamArchiver::HamNode& rhs) {
    return lhs.freq_ < rhs.freq_;
}

bool operator>(const HamArchiver::HamNode& lhs, const HamArchiver::HamNode& rhs) {
    return lhs.freq_ > rhs.freq_;
}

bool operator<=(const HamArchiver::HamNode& lhs, const HamArchiver::HamNode& rhs) {
    return lhs.freq_ <= rhs.freq_;
}

bool operator>=(const HamArchiver::HamNode& lhs, const HamArchiver::HamNode& rhs) {
    return lhs.freq_ >= rhs.freq_;
}

bool operator==(const HamArchiver::HamNode& lhs, const HamArchiver::HamNode& rhs) {
    return lhs.freq_ == rhs.freq_;
}

uint64_t HamArchiver::Power(uint64_t left, uint8_t right) {
    uint64_t result = 1;
    for (size_t i = 0; i < right; i++) {
        result *= left;
    }
    return result;
}

void HamArchiver::AddEncodedByteToArchive(uint8_t current_byte, std::ofstream& arch) {
    bool current_bits[CHAR_BIT];
    size_t control_bit;
    std::vector<bool> hamming_code(CHAR_BIT, false);

    for (size_t j = 0; j < CHAR_BIT; j++) {
        current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
    }

    for (uint16_t bits_count: HamArchiver::kBits) {
        control_bit = 1;
        for (size_t i = 0; i < CHAR_BIT; i++) {
            if (control_bit - 1 == i) {
                control_bit <<= 1;
            } else {
                hamming_code[i] = current_bits[bits_count];
                bits_count++;
            }
        }
        
        HamEncoder::EncodeHamming(hamming_code);
        current_byte = 0;
        
        for (int i = 0; i < CHAR_BIT; i++) {
            if (hamming_code[i]) {
                current_byte += static_cast<uint8_t>(Power(2, CHAR_BIT - i - 1));
            }
        }
        arch << current_byte;
    }
}

void HamArchiver::AddDecodedHammingToFile(const File& file, std::ofstream& output) {
    uint8_t current_byte;
    std::ifstream file_stream;
    file_stream.open(file.name_, std::ios::binary);

    if (file.control_bits_ == 3) {
        for (size_t i = 0; i < file.bytes_count_; i++) {
            current_byte = file_stream.get();
            AddEncodedByteToArchive(current_byte, output);
        }
    } else if (file.control_bits_ >= 4) {
        uint64_t hamming_bits_count = Power(2, file.control_bits_);
        uint64_t info_bits_count = hamming_bits_count - file.control_bits_ - 1;
        
        std::vector<bool> cur_hamming_bits(hamming_bits_count, false);
        std::vector<bool> next_hamming_bits(CHAR_BIT, false);
        
        uint64_t prev_bits_count = 0;
        uint64_t control_bit = 1;
        uint64_t current_bit = 0;
        uint64_t next_bits_count = 0;
        uint64_t cur_bytes_count = 0;
        uint64_t bytes_to_add = 0;

        bool current_bits[CHAR_BIT];

        while ((cur_bytes_count < file.bytes_count_) || (prev_bits_count != 0)) {
            bytes_to_add = (info_bits_count - prev_bits_count) / 8 +
                    ((info_bits_count - prev_bits_count) % 8 != 0);

            for (size_t i = 0; (i < bytes_to_add) && (cur_bytes_count < file.bytes_count_); i++) {
                cur_bytes_count++;
                current_byte = file_stream.get();

                for (size_t j = 0; j < CHAR_BIT; j++) {
                    current_bits[CHAR_BIT - j - 1] = (current_byte >> j) & 1;
                }

                for (size_t j = 0; j < CHAR_BIT; j++) {
                    while (current_bit == control_bit - 1) {
                        current_bit++;
                        control_bit <<= 1;
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
                        current_byte += static_cast<uint8_t>(Power(2, CHAR_BIT - j - 1));
                    }
                }
                output << current_byte;

            }

            for (uint64_t i = 0; i < hamming_bits_count; i++) {
                cur_hamming_bits[i] = false;
            }

            current_bit = 0;
            control_bit = 1;
            for (int i = 0; i < next_bits_count; i++) {
                while (current_bit == control_bit - 1) {
                    current_bit++;
                    control_bit <<= 1;
                }
                cur_hamming_bits[current_bit++] = next_hamming_bits[i];
            }
            prev_bits_count = next_bits_count;
            next_bits_count = 0;
        }
    }
}