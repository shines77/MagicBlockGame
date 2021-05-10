#pragma once

#include "MagicBlock/AI/Color.h"

namespace MagicBlock {
namespace AI {

struct ErrorCode {
    enum ErrorType {
        ErrorFirst = -10000,
        TargetBoardColorOverflowFirst = -200,
        TargetBoardColorOverflowLast = TargetBoardColorOverflowFirst + Color::Maximum - 1,
        PlayerBoardColorOverflowFirst = TargetBoardColorOverflowFirst + Color::Maximum,
        PlayerBoardColorOverflowLast = PlayerBoardColorOverflowFirst + Color::Maximum - 1,
        TargetBoardColorOverflow = -7,
        PlayerBoardColorOverflow = -6,
        UnknownTargetBoardColor = -5,
        UnknownPlayerBoardColor = -4,
        ifstream_IsFailed = -3,
        ifstream_IsBad = -2,
        StdException = -1,
        Success = 0,
    };

    static bool isFailure(int errCode) {
        return (errCode < 0);
    }

    static bool isSuccess(int errCode) {
        return (errCode >= 0);
    }

    static const char * toString(int errCode) {
        switch (errCode) {
            case ErrorType::TargetBoardColorOverflow:
                return "Target board color overflow";
            case ErrorType::PlayerBoardColorOverflow:
                return "Player board color overflow";
            case ErrorType::UnknownTargetBoardColor:
                return "Unknown target board color";
            case ErrorType::UnknownPlayerBoardColor:
                return "Unknown player board color";
            case ErrorType::StdException:
                return "It's a std::exception";
            case ErrorType::Success:
                return "Success";
            default:
                return "Unknown error code";
        }
    }

    static const char * toErrorString(int errCode) {
        switch (errCode) {
            case ErrorType::TargetBoardColorOverflow:
                return "Error: Target board color overflow";
            case ErrorType::PlayerBoardColorOverflow:
                return "Error: Player board color overflow";
            case ErrorType::UnknownTargetBoardColor:
                return "Error: Unknown target board color";
            case ErrorType::UnknownPlayerBoardColor:
                return "Error: Unknown player board color";
            case ErrorType::StdException:
                return "Error: It's a std::exception";
            case ErrorType::Success:
                return "Success";
            default:
                return "Error: Unknown error code";
        }
    }
};

} // namespace AI
} // namespace MagicBlock
