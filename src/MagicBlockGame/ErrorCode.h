#pragma once

namespace PuzzleGame {

struct ErrorCode {
    enum ErrorType {
        ErrorFirst = -10000,
        UnknownTargetColor,
        UnknownBoardColor,
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
            case ErrorType::UnknownTargetColor:
                return "Unknown target color";
            case ErrorType::UnknownBoardColor:
                return "Unknown board color";
            case ErrorType::StdException:
                return "It's a std::exception";
            case ErrorType::Success:
                return "Success";
            default:
                return "Unknown error code";
        }
    }

    static const char * toStatusString(int errCode) {
        switch (errCode) {
            case ErrorType::UnknownTargetColor:
                return "Error: Unknown target color";
            case ErrorType::UnknownBoardColor:
                return "Error: Unknown board color";
            case ErrorType::StdException:
                return "Error: It's a std::exception";
            case ErrorType::Success:
                return "Success";
            default:
                return "Error: Unknown error code";
        }
    }
};

} // namespace PuzzleGame
