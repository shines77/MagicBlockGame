#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstring>      // For std::memset()
#include <vector>
#include <type_traits>  // For std::conditional<bool, T1, T2>
#include <algorithm>    // For std::fill_n()

#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Value128.h"

namespace MagicBlock {
namespace AI {

template <std::size_t BoardX, std::size_t BoardY>
union Board
{
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    static const size_type X = BoardX;
    static const size_type Y = BoardY;
    static const size_type BoardSize = BoardX * BoardY;
    static const size_type Bytes = BoardX * BoardY;

    typedef typename std::conditional<
                (Bytes <= sizeof(std::uint32_t)), std::uint32_t, size_type
            >::type  unit_type;
    typedef Board<BoardX, BoardY> this_type;

    static const size_type kUnitBytes = sizeof(unit_type);

    static const size_type kUnits = (Bytes + kUnitBytes - 1) / kUnitBytes;
    static const size_type kBytes = kUnits * kUnitBytes;

    std::uint8_t    cells[BoardSize];
    unit_type       uints[kUnits];

    Board() {
        this->clear();
    }
    Board(const Board & src) noexcept {
        this->internal_copy(src);
    }
    Board(Board && src) noexcept {
        this->internal_swap(src);
    }

    ~Board() {}

    Board & operator = (const Board & rhs) noexcept {
        this->copy(rhs);
        return *this;
    }

    Board & operator = (Board && rhs) noexcept {
        this->swap(rhs);
        return *this;
    }

    void clear() noexcept {
#if 0
        std::fill_n(this->cells, sizeof(cells), Color::Empty);
#else
        for (size_type n = 0; n < kUnits; n++) {
            this->uints[n] = 0;
        }
#endif
    }

    void internal_copy(const Board & other) noexcept {
        for (size_type n = 0; n < kUnits; n++) {
            this->uints[n] = other.uints[n];
        }
    }

    void copy(const Board & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void copy(uint8_t cells[BoardSize]) noexcept {
        size_type cell;
        for (cell = 0; cell < BoardSize; cell++) {
            this->cells[cell] = cells[cell];
        }
        for (; cell < kBytes; cell++) {
            this->cells[cell] = 0;
        }
    }

    void internal_swap(Board & other) noexcept {
        for (size_type n = 0; n < kUnits; n++) {
            unit_type temp = this->uints[n];
            this->uints[n] = other.uints[n];
            other.uints[n] = temp;
        }
    }

    void swap(Board & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    bool is_equal(const Board & other) const noexcept {
#if 1
        for (size_type n = 0; n < kUnits; n++) {
            if (this->uints[n] != other.uints[n])
                return false;
        }
#else
        for (size_type cell = 0; cell < BoardSize; cell++) {
            if (this->cells[cell] != other.cells[cell])
                return false;
        }
#endif
        return true;
    }

    bool find_empty(Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t color = this->cells[y * BoardX + x];
                if (color == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardX + x);
                    return true;
                }
            }
        }
        return false;
    }

    size_type value() const noexcept {
        size_type value = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            value <<= 3;
            value |= (this->cells[pos] & 0x07U);
        }
        return value;
    }

    std::uint64_t value64() const noexcept {
        std::uint64_t value64 = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            value64 <<= 3;
            value64 |= (this->cells[pos] & 0x07U);
        }
        return value64;
    }

    template <size_type kEmptyPosValue = 0>
    size_type compactValue() const noexcept {
        size_type value = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            if (this->cells[pos] != kEmptyPosValue) {
                value <<= 3;
                value |= (this->cells[pos] & 0x07U);
            }
        }
        return value;
    }

    template <size_type kEmptyPosValue = 0>
    std::uint64_t compactValue64() const noexcept {
        std::uint64_t value64 = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            if (this->cells[pos] != kEmptyPosValue) {
                value64 <<= 3;
                value64 |= (this->cells[pos] & 0x07U);
            }
        }
        return value64;
    }

    Value128 value128() const noexcept {
        std::uint64_t low = 0, high = 0;
        if (BoardSize <= 21) {
            for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
                low <<= 3;
                low |= std::uint64_t(this->cells[pos] & 0x07U);
            }
        }
        else {
            // Low: bit 0 ~ 62, 21 * 3 = 63 bits
            for (ssize_type pos = 20; pos >= 0; pos--) {
                low <<= 3;
                low |= std::uint64_t(this->cells[pos] & 0x07U);
            }
            // Low: bit 63
            low |= std::uint64_t(this->cells[21] & 0x01U) << 63;

            // High: bit 2 ~ 63
            for (ssize_type pos = BoardSize - 1; pos >= 21; pos--) {
                high <<= 3;
                high |= std::uint64_t(this->cells[pos] & 0x07U);
            }

            // High: bit 0 ~ 1
            high >>= 1;
        }
        return Value128(low, high);
    }

    // clockwise rotate 90 degrees
    void rotate_90_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = (BoardY - 1) - y;
                size_type dest_y = x;
                size_type dest_pos = dest_y * BoardX + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 180 degrees
    void rotate_180_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = (BoardX - 1) - x;
                size_type dest_y = (BoardY - 1) - y;
                size_type dest_pos = dest_y * BoardX + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    // clockwise rotate 270 degrees
    void rotate_270_cw(Board<BoardX, BoardY> & dest) {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = y;
                size_type dest_y = (BoardX - 1) - x;
                size_type dest_pos = dest_y * BoardX + dest_x;
                dest.cells[dest_pos] = this->cells[src_pos];
            }
        }
    }

    static void display_board(const char * title, const this_type & board) {
        printf("%s\n\n", title);
        // -------
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n");
        for (size_type y = 0; y < BoardY; y++) {
            printf(" ");
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t color = board.cells[y * BoardX + x];
                assert(color >= Color::First && color < Color::Maximum);
                printf("%s ", Color::colorToChar(color));
            }
            printf("\n");
        }
        // -------
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n\n");
    }

    template <size_type EmptyPosValue>
    static void display_num_board(const char * title, const this_type & board) {
        printf("%s\n\n", title);
        // -------
        printf(" ");
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n");
        for (size_type y = 0; y < BoardY; y++) {
            printf("| ");
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t num = board.cells[y * BoardX + x];
                assert((num == EmptyPosValue) || (num >= 0 && num < BoardSize));
                printf("%c ", (num != EmptyPosValue) ? (num + '1') : '0');
            }
            printf("|\n");
        }
        // -------
        printf(" ");
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n\n");
    }

    template <size_type EmptyPosValue, size_type UnknownPosValue>
    static void display_num_board(const char * title, const this_type & board) {
        printf("%s\n\n", title);
        // -------
        printf(" ");
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n");
        for (size_type y = 0; y < BoardY; y++) {
            printf("| ");
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t num = board.cells[y * BoardX + x];
                assert((num == UnknownPosValue) || (num == EmptyPosValue) || (num >= 0 && num < BoardSize));
                printf("%c ", (num != UnknownPosValue) ? ((num != EmptyPosValue) ? (num + '1') : '0') : '?');
            }
            printf("|\n");
        }
        // -------
        printf(" ");
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n\n");
    }

    static void display_boards(const char * title, const std::vector<this_type> & board_list) {
        for (size_type n = 0; n < board_list.size(); n++) {
            char title_no[128];
            snprintf(title_no, sizeof(title_no), "%s #%u", title, (uint32_t)(n + 1));
            this_type::display_board(title, board_list[n]);
        }
    }

    template <size_type EmptyPosValue>
    static void display_num_boards(const char * title, const std::vector<this_type> & board_list) {
        for (size_type n = 0; n < board_list.size(); n++) {
            char title_no[128];
            snprintf(title_no, sizeof(title_no), "%s #%u", title, (uint32_t)(n + 1));
            this_type::template display_num_board<EmptyPosValue>(title_no, board_list[n]);
        }
    }

    template <size_type EmptyPosValue, size_type UnknownPosValue>
    static void display_num_boards(const char * title, const std::vector<this_type> & board_list) {
        for (size_type n = 0; n < board_list.size(); n++) {
            char title_no[128];
            snprintf(title_no, sizeof(title_no), "%s #%u", title, (uint32_t)(n + 1));
            this_type::template display_num_board<EmptyPosValue, UnknownPosValue>(title_no, board_list[n]);
        }
    }

    template <size_type UBoardX, size_type UBoardY>
    static bool translate_move_path(const Board<UBoardX, UBoardY> & in_board,
                                    const std::vector<Position> & move_path,
                                    std::vector<MoveInfo> & answer,
                                    Position in_empty_pos = std::uint8_t(-1)) {
        bool success = true;

        answer.clear();

        Board<UBoardX, UBoardY> board(in_board);
        std::uint8_t from_pos, move_to_pos;
        std::uint8_t from_clr, move_to_clr;
        std::uint8_t last_dir = std::uint8_t(-1);
        Position empty_pos = in_empty_pos;
        assert((empty_pos.value == std::uint8_t(-1)) || (board.cells[empty_pos] == Color::Unknown));
        if (empty_pos.value != std::uint8_t(-1)) {
            board.cells[empty_pos] = Color::Empty;
        }
        bool found_empty = board.find_empty(empty_pos);
        if (!found_empty) {
            empty_pos = std::uint8_t(-1);
        }
        move_to_pos = empty_pos;
        for (size_type i = 0; i < move_path.size(); i++) {
            if (move_to_pos != std::uint8_t(-1))
                move_to_clr = board.cells[move_to_pos];
            else
                move_to_clr = Color::Unknown;
            from_pos = move_path[i].value;
            from_clr = board.cells[from_pos];
            if (from_clr != Color::Empty && move_to_clr == Color::Empty) {
                last_dir = Direction::template getDir<UBoardX, UBoardY>(from_pos, move_to_pos);
                MoveInfo move_info;
                move_info.from_pos = from_pos;
                move_info.to_pos = move_to_pos;
                move_info.color = from_clr;
                move_info.dir = last_dir;
                answer.push_back(move_info);

                std::swap(board.cells[from_pos], board.cells[move_to_pos]);
            }
            else {
                printf("Board<X, Y>::translate_move_path():\n\n"
                        "Move path have error, [from_pos] is a empty gird.\n"
                        "index = %u, from_pos = %c%c, color = %s (%u)\n\n",
                        (std::uint32_t)(i + 1),
                        Position::template toFirstChar<UBoardX>(from_pos),
                        Position::template toSecondChar<UBoardX>(from_pos),
                        Color::colorToChar(from_clr),
                        (std::uint32_t)from_clr);
                success = false;
                break;
            }
            move_to_pos = from_pos;
        }

        return success;
    }

    bool translate_move_path(const std::vector<Position> & move_path,
                             std::vector<MoveInfo> & answer,
                             Position empty_pos = std::uint8_t(-1)) const {
        return this_type::template translate_move_path<BoardX, BoardY>(*this, move_path, answer, empty_pos);
    }

    template <size_type UBoardX>
    static void display_answer(const std::vector<MoveInfo> & answer) {
        size_type index = 0;
        printf("Answer_Move_Path[%u] = {\n", (std::uint32_t)answer.size());
        for (auto iter : answer) {
            Position from_pos   = iter.from_pos;
            Position to_pos     = iter.to_pos;
            size_type color     = iter.color;
            size_type dir       = iter.dir;
            printf("    [%2u]: [%s], %c%c --> %c%c, dir: %-5s (%u)\n",
                   (std::uint32_t)(index + 1),
                   Color::colorToChar(color),
                   Position::template toFirstChar<UBoardX>(from_pos),
                   Position::template toSecondChar<UBoardX>(from_pos),
                   Position::template toFirstChar<UBoardX>(to_pos),
                   Position::template toSecondChar<UBoardX>(to_pos),
                   Direction::toString(dir),
                   (std::uint32_t)dir);
            index++;
        }
        printf("};\n\n");
    }

    void display_answer(const std::vector<MoveInfo> & answer) const {
        this_type::template display_answer<BoardX>(answer);
    }
};

template <std::size_t BoardX, std::size_t BoardY>
inline
bool operator == (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept {
    return lhs.is_equal(rhs);
}

template <std::size_t BoardX, std::size_t BoardY>
inline
bool operator != (const Board<BoardX, BoardY> & lhs, const Board<BoardX, BoardY> & rhs) noexcept {
    return !(lhs.is_equal(rhs));
}

template <std::size_t BoardX, std::size_t BoardY>
inline
void swap(Board<BoardX, BoardY> & lhs, Board<BoardX, BoardY> & rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace AI
} // namespace MagicBlock
