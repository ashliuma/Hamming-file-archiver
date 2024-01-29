#pragma once

#include <vector>
#include <iostream>

namespace HamEncoder {
    void EncodeHamming(std::vector<bool> &code);

    bool DecodeHamming(std::vector<bool> &code);
};
