#pragma once

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

template <std::size_t EmptyPosValue = Color::Empty,
          std::size_t UnknownPosValue = Color::Unknown>
struct Number {
    static const std::uint8_t kEmptyPosValue    = (std::uint8_t)EmptyPosValue;
    static const std::uint8_t kUnknownPosValue  = (std::uint8_t)UnknownPosValue;
    static const std::uint8_t kInvalidValue     = (std::uint8_t)-1;

    static std::uint8_t toNumber(std::uint8_t ascii) {
        if (ascii >= '1' && ascii <= '9') {
            std::uint8_t num = (ascii - '1');
            return ((num < kEmptyPosValue) ? num : kInvalidValue);
        }
        else if (ascii >= 'A' && ascii <= 'Z') {
            std::uint8_t num = (ascii - 'A');
            return ((num < kEmptyPosValue) ? num : kInvalidValue);
        }
        else if (ascii == ' ' || ascii == '0')
            return kEmptyPosValue;
        else if (ascii == '?')
            return kUnknownPosValue;
        else
            return kInvalidValue;
    }

    static char toChar(std::size_t num) {
        if (num == UnknownPosValue) {
            return '?';
        }
        else if (num == EmptyPosValue) {
            return '0';
        }
        else if (num < EmptyPosValue && num < 10) {
            return (char)('1' + num);
        }
        else {
            return '*';
        }
    }

    static const char * toString(std::size_t num) {
        if (num == UnknownPosValue) {
            return "?";
        }
        else if (num == EmptyPosValue) {
            return "0";
        }
        else if (num < EmptyPosValue) {
#if 1
            static char num_buf[32] = { 0 };
            ::itoa(num + 1, num_buf, 10);
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
