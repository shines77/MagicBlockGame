#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <cstring>

#include "MagicBlock/AI/Color.h"

namespace MagicBlock {
namespace AI {

template <std::size_t EmptyColor = Color::Empty,
          std::size_t UnknownColor = Color::Unknown>
struct Number {
    static const std::uint8_t kEmptyColor    = (std::uint8_t)EmptyColor;
    static const std::uint8_t kUnknownColor  = (std::uint8_t)UnknownColor;
    static const std::uint8_t kInvalidValue  = (std::uint8_t)-1;

    static std::uint8_t toNumber(std::uint8_t ascii) {
        if (ascii >= '1' && ascii <= '9') {
            std::uint8_t num = (ascii - '1');
            return ((num < kEmptyColor) ? num : kInvalidValue);
        }
        else if (ascii >= 'A' && ascii <= 'Z') {
            std::uint8_t num = (ascii - 'A');
            return ((num < kEmptyColor) ? num : kInvalidValue);
        }
        else if (ascii == ' ' || ascii == '0')
            return kEmptyColor;
        else if (ascii == '?')
            return kUnknownColor;
        else
            return kInvalidValue;
    }

    static char toChar(std::size_t num) {
        if (num == UnknownColor) {
            return '?';
        }
        else if (num == EmptyColor) {
            return '0';
        }
        else if (num < EmptyColor && num < 10) {
            return (char)('1' + num);
        }
        else {
            return '*';
        }
    }

    static const char * toString(std::size_t num) {
        if (num == UnknownColor) {
            return "?";
        }
        else if (num == EmptyColor) {
            return "0";
        }
        else if (num < EmptyColor) {
#if 1
            static char num_buf[32];
            snprintf(num_buf, sizeof(num_buf), "%u", (std::uint32_t)(num + 1));
            return num_buf;
#else
            static std::string strNum;       
            strNum = std::to_string(num + 1);
            return strNum.c_str();
#endif
        }
        else {
            return "*";
        }
    }
};

} // namespace AI
} // namespace MagicBlock
