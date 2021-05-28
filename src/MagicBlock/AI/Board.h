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
#include "MagicBlock/AI/Number.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/MoveSeq.h"
#include "MagicBlock/AI/Value128.h"

namespace MagicBlock {
namespace AI {

#pragma pack(push, 1)

template <std::size_t BoardX, std::size_t BoardY>
union Board
{
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  ssize_type;

    static const size_type X = BoardX;
    static const size_type Y = BoardY;
    static const size_type BoardSize = BoardX * BoardY;
    static const size_type TotalBytes = BoardX * BoardY;

#if 1
    typedef std::uint32_t unit_type;

    typedef typename std::conditional<
                (TotalBytes <= sizeof(std::uint32_t)), std::uint32_t, size_type
            >::type  auto_type;
#else
    typedef typename std::conditional<
                (TotalBytes <= sizeof(std::uint32_t)), std::uint32_t, size_type
            >::type  unit_type;

    typedef unit_type auto_type;
#endif

    typedef Board<BoardX, BoardY>   this_type;

    typedef MoveSeq                 move_seq_t;
    typedef std::vector<MoveInfo>   move_list_t;

    static const size_type kUnitBytes = sizeof(unit_type);

    static const size_type kTotalUnits = (TotalBytes + kUnitBytes - 1) / kUnitBytes;
    static const size_type kTotalBytes = kTotalUnits * kUnitBytes;

    static const size_type kAutoUnitBytes = sizeof(auto_type);
    static const size_type kTotalAutoUnits = TotalBytes / kAutoUnitBytes;
    static const size_type kTotalAutoBytes = kTotalAutoUnits * kAutoUnitBytes;
    static const size_type kRemainAutoBytes = kTotalBytes - kTotalAutoBytes;

    static const size_type kRemainUnits = (kRemainAutoBytes + kUnitBytes - 1) / kUnitBytes;

    std::uint8_t    cells[BoardSize];
    unit_type       units[kTotalUnits];

    Board() {
        this->clear();
    }
    Board(const Board & src) noexcept {
        this->internal_copy(src);
    }

    ~Board() {}

    Board & operator = (const Board & rhs) noexcept {
        this->copy(rhs);
        return *this;
    }

    bool is_x64() const {
        return (sizeof(std::size_t) == 8);
    }

    bool size() const {
        return this->BoardSize;
    }

    bool capacity() const {
        return this->kTotalBytes;
    }

    bool board_size() const {
        return this->BoardSize;
    }

    bool unit_size() const {
        return this->kTotalUnits;
    }

    std::uint8_t * data() {
        return (std::uint8_t *)&this->units[0];
    }

    const std::uint8_t * data() const {
        return (const std::uint8_t *)&this->units[0];
    }

    unit_type * unit_data() {
        return (unit_type *)&this->units[0];
    }

    const unit_type * unit_data() const {
        return (const unit_type *)&this->units[0];
    }

    auto_type * auto_data() {
        return (auto_type *)&this->units[0];
    }

    const auto_type * auto_data() const {
        return (const auto_type *)&this->units[0];
    }

    void clear() noexcept {
        if (this->is_x64()) {
            auto_type * auto_units = this->auto_data();
            for (size_type n = 0; n < kTotalAutoUnits; n++) {
                *auto_units++ = 0;
            }
            if (kRemainUnits != 0) {
                unit_type * _units = (unit_type *)auto_units;
                for (size_type n = 0; n < kRemainUnits; n++) {
                    *_units++ = 0;
                    if (n < kRemainUnits - 1)
                        _units++;
                }
            }
        }
        else {
#if 0
            std::fill_n(this->uints, kTotalUnits, unit_type(0));
#else
            for (size_type n = 0; n < kTotalUnits; n++) {
                this->units[n] = 0;
            }
#endif
        }
    }

    void fill(std::uint8_t color) noexcept {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            this->cells[pos] = color;
        }
    }

    void fill_all(std::uint8_t color) noexcept {
        size_type pos;
        for (pos = 0; pos < BoardSize; pos++) {
            this->cells[pos] = color;
        }
        for (; pos < kTotalBytes; pos++) {
            this->cells[pos] = 0;
        }
    }

    void internal_copy(const Board & other) noexcept {
        if (this->is_x64()) {
                  auto_type * auto_units       = this->auto_data();
            const auto_type * other_auto_units = other.auto_data();
            for (size_type n = 0; n < kTotalAutoUnits; n++) {
                *auto_units++ = *other_auto_units++;
            }
            if (kRemainUnits != 0) {
                unit_type * _units       = (unit_type *)auto_units;
                unit_type * _other_units = (unit_type *)other_auto_units;
                for (size_type n = 0; n < kRemainUnits; n++) {
                    *_units = *_other_units;
                    if (n < kRemainUnits - 1) {
                        _units++;
                        _other_units++;
                    }
                }
            }
        }
        else {
            for (size_type n = 0; n < kTotalUnits; n++) {
                this->units[n] = other.units[n];
            }
        }
    }

    void internal_swap(Board & other) noexcept {
        if (this->is_x64()) {
            auto_type * auto_units       = this->auto_data();
            auto_type * other_auto_units = other.auto_data();
            for (size_type n = 0; n < kTotalAutoUnits; n++) {
                auto_type temp = *auto_units;
                *auto_units = *other_auto_units;
                *other_auto_units = temp;
                auto_units++;
                other_auto_units++;
            }
            if (kRemainUnits != 0) {
                unit_type * _units       = (unit_type *)auto_units;
                unit_type * _other_units = (unit_type *)other_auto_units;
                for (size_type n = 0; n < kRemainUnits; n++) {
                    unit_type temp = *_units;
                    *_units = *_other_units;
                    *_other_units = temp;
                    if (n < kRemainUnits - 1) {
                        _units++;
                        _other_units++;
                    }
                }
            }
        }
        else {
            for (size_type n = 0; n < kTotalUnits; n++) {
                unit_type temp = this->units[n];
                this->units[n] = other.units[n];
                other.units[n] = temp;
            }
        }
    }

    void copy(const Board & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void copy(uint8_t cells[BoardSize]) noexcept {
        size_type pos;
        for (pos = 0; pos < BoardSize; pos++) {
            this->cells[pos] = cells[pos];
        }
        for (; pos < kTotalBytes; pos++) {
            this->cells[pos] = 0;
        }
    }

    void swap(Board & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    bool is_equal(const Board & other) const noexcept {
        if (this->is_x64()) {
            const auto_type * auto_units       = this->auto_data();
            const auto_type * other_auto_units = other.auto_data();
            for (size_type n = 0; n < kTotalAutoUnits; n++) {
                if (*auto_units++ != *other_auto_units++)
                    return false;
            }
            if (kRemainUnits != 0) {
                unit_type * _units       = (unit_type *)auto_units;
                unit_type * _other_units = (unit_type *)other_auto_units;
                for (size_type n = 0; n < kRemainUnits; n++) {
                    if (*_units != *_other_units)
                        return false;
                    if (n < kRemainUnits - 1) {
                        _units++;
                        _other_units++;
                    }
                }
            }
        }
        else {
            for (size_type n = 0; n < kTotalUnits; n++) {
                if (this->units[n] != other.units[n])
                    return false;
            }
        }
        return true;
    }

    bool find_empty(Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t clr = this->cells[y * BoardX + x];
                if (clr == Color::Empty) {
                    empty_pos = (uint8_t)(y * BoardX + x);
                    return true;
                }
            }
        }
        return false;
    }

    template <size_type EmptyColor = Color::Empty>
    bool find_empty(Position & empty_pos) const {
        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t clr = this->cells[y * BoardX + x];
                if (clr == EmptyColor) {
                    empty_pos = (uint8_t)(y * BoardX + x);
                    return true;
                }
            }
        }
        return false;
    }

    template <size_type TargetColor = Color::Empty>
    bool find_color(Position & target_pos) const {
        for (size_type pos = 0; pos < BoardSize; pos++) {
            uint8_t clr = this->cells[pos];
            if (clr == TargetColor) {
                target_pos = pos;
                return true;
            }
        }
        return false;
    }

    template <size_type TargetColor = Color::Empty>
    bool find_color(size_type start_pos, Position & target_pos) const {
        for (size_type pos = start_pos; pos < BoardSize; pos++) {
            uint8_t clr = this->cells[pos];
            if (clr == TargetColor) {
                target_pos = pos;
                return true;
            }
        }
        return false;
    }

    template <size_type TargetColor = Color::Empty>
    void find_all_color(std::vector<Position> & pos_list) const {
        pos_list.clear();
        for (size_type pos = 0; pos < BoardSize; pos++) {
            uint8_t clr = this->cells[pos];
            if (clr == TargetColor) {
                pos_list.push_back(pos);
            }
        }
    }

    template <size_type EmptyColor = Color::Empty>
    bool find_empty(size_type start_pos, Position & empty_pos) const {
        return this->template find_color<EmptyColor>(start_pos, empty_pos);
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

    template <size_type kEmptyColor = Color::Empty>
    size_type compactValue() const noexcept {
        size_type value = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            if (this->cells[pos] != kEmptyColor) {
                value <<= 3;
                value |= (this->cells[pos] & 0x07U);
            }
        }
        return value;
    }

    template <size_type kEmptyColor = Color::Empty>
    std::uint64_t compactValue64() const noexcept {
        std::uint64_t value64 = 0;
        for (ssize_type pos = BoardSize - 1; pos >= 0; pos--) {
            if (this->cells[pos] != kEmptyColor) {
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
    void rotate_90() {
        Board<BoardX, BoardY> copy(*this);

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = (BoardY - 1) - y;
                size_type dest_y = x;
                size_type dest_pos = dest_y * BoardX + dest_x;
                this->cells[dest_pos] = copy.cells[src_pos];
            }
        }
    }

    // clockwise rotate 180 degrees
    void rotate_180() {
        Board<BoardX, BoardY> copy(*this);

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = (BoardX - 1) - x;
                size_type dest_y = (BoardY - 1) - y;
                size_type dest_pos = dest_y * BoardX + dest_x;
                this->cells[dest_pos] = copy.cells[src_pos];
            }
        }
    }

    // clockwise rotate 270 degrees
    void rotate_270() {
        Board<BoardX, BoardY> copy(*this);

        for (size_type y = 0; y < BoardY; y++) {
            for (size_type x = 0; x < BoardX; x++) {
                size_type src_pos = y * BoardX + x;
                size_type dest_x = y;
                size_type dest_y = (BoardX - 1) - x;
                size_type dest_pos = dest_y * BoardX + dest_x;
                this->cells[dest_pos] = copy.cells[src_pos];
            }
        }
    }

    // clockwise rotate 90 degrees
    void rotate_to_90(Board<BoardX, BoardY> & dest) {
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
    void rotate_to_180(Board<BoardX, BoardY> & dest) {
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
    void rotate_to_270(Board<BoardX, BoardY> & dest) {
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
        printf(" ");
        for (size_type x = 0; x < BoardX * 2 + 1; x++) {
            printf("-");
        }
        printf("\n");
        for (size_type y = 0; y < BoardY; y++) {
            printf("| ");
            for (size_type x = 0; x < BoardX; x++) {
                uint8_t color = board.cells[y * BoardX + x];
                assert(color >= Color::First && color < Color::Maximum);
                printf("%c ", Color::toChar(color));
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

    template <size_type EmptyColor>
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
                assert((num == EmptyColor) || (num >= 0 && num < BoardSize));
                printf("%c ", (num != EmptyColor) ? (num + '1') : '0');
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

    template <size_type EmptyColor, size_type UnknownColor>
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
                assert((num == UnknownColor) || (num == EmptyColor) || (num >= 0 && num < BoardSize));
                printf("%c ", (num != UnknownColor) ? ((num != EmptyColor) ? (num + '1') : '0') : '?');
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

    template <size_type EmptyColor, size_type UnknownColor>
    static void display_num_board(const char * title, size_type index, const this_type & board) {
        printf("%s #%d\n\n", title, (int)(index + 1));
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
                assert((num == UnknownColor) || (num == EmptyColor) || (num >= 0 && num < BoardSize));
                printf("%c ", (num != UnknownColor) ? ((num != EmptyColor) ? (num + '1') : '0') : '?');
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

    template <size_type EmptyColor>
    static void display_num_boards(const char * title, const std::vector<this_type> & board_list) {
        for (size_type n = 0; n < board_list.size(); n++) {
            char title_no[128];
            snprintf(title_no, sizeof(title_no), "%s #%u", title, (uint32_t)(n + 1));
            this_type::template display_num_board<EmptyColor>(title_no, board_list[n]);
        }
    }

    template <size_type EmptyColor, size_type UnknownColor>
    static void display_num_boards(const char * title, const std::vector<this_type> & board_list) {
        for (size_type n = 0; n < board_list.size(); n++) {
            char title_no[128];
            snprintf(title_no, sizeof(title_no), "%s #%u", title, (uint32_t)(n + 1));
            this_type::template display_num_board<EmptyColor, UnknownColor>(title_no, board_list[n]);
        }
    }

    template <size_type EmptyColor = Color::Empty, size_type UnknownColor = Color::Unknown>
    static bool translate_move_seq(const this_type & in_board,
                                   const move_seq_t & move_seq,
                                   move_list_t & move_list,
                                   Position in_empty_pos = std::uint8_t(-1)) {
        bool success = true;

        move_list.clear();

        this_type board(in_board);
        std::uint8_t move_pos;
        std::uint8_t move_clr, empty_clr;
        std::uint8_t move_dir = std::uint8_t(-1);
        Position empty_pos = in_empty_pos;
        assert((empty_pos.value == std::uint8_t(-1)) || (board.cells[empty_pos] == UnknownColor));
        if (empty_pos.value != std::uint8_t(-1)) {
            board.cells[empty_pos] = EmptyColor;
        }
        bool found_empty = board.find_empty<EmptyColor>(empty_pos);
        if (!found_empty) {
            empty_pos = std::uint8_t(-1);
        }
        for (size_type i = 0; i < move_seq.size(); i++) {
            if (empty_pos.value != std::uint8_t(-1))
                empty_clr = board.cells[empty_pos];
            else
                empty_clr = UnknownColor;
            move_dir = move_seq[i];
            move_pos = Dir::template getMovePos<BoardX, BoardY>(move_dir, empty_pos);
            move_clr = board.cells[move_pos];
            if (move_clr != EmptyColor && empty_clr == EmptyColor) {
                MoveInfo move_info;
                move_info.from_pos  = move_pos;
                move_info.to_pos    = empty_pos;
                move_info.color     = move_clr;
                move_info.dir       = move_dir;
                move_list.push_back(move_info);

                std::swap(board.cells[move_pos], board.cells[empty_pos]);
            }
            else {
                printf("Board<X, Y>::translate_move_seq():\n\n"
                        "Move sequence have error, [move_pos] is a empty grid.\n"
                        "index = %u, move_pos = %c%c, color = %c (%u)\n\n",
                        (std::uint32_t)(i + 1),
                        Position::template toFirstChar<BoardX>(move_pos),
                        Position::template toSecondChar<BoardX>(move_pos),
                        Color::toChar(move_clr),
                        (std::uint32_t)move_clr);
                success = false;
                break;
            }
            empty_pos = move_pos;
        }

        return success;
    }

    template <size_type EmptyColor = Color::Empty, size_type UnknownColor = Color::Unknown>
    bool translate_move_seq(const move_seq_t & move_seq,
                            move_list_t & move_list,
                            Position empty_pos = std::uint8_t(-1)) const {
        return this_type::translate_move_seq<EmptyColor, UnknownColor>(*this, move_seq, move_list, empty_pos);
    }

    static void display_move_list(const move_list_t & move_list) {
        size_type index = 0;
        printf("Answer_Move_Path[ %u ] = {\n", (std::uint32_t)move_list.size());
        for (auto iter : move_list) {
            Position from_pos   = iter.from_pos;
            Position to_pos     = iter.to_pos;
            size_type color     = iter.color;
            size_type dir       = iter.dir;
            printf("    [%2u]: [%c], %c%c --> %c%c, dir: %-5s (%u)\n",
                   (std::uint32_t)(index + 1),
                   Color::toChar(color),
                   Position::template toFirstChar<BoardX>(from_pos),
                   Position::template toSecondChar<BoardX>(from_pos),
                   Position::template toFirstChar<BoardX>(to_pos),
                   Position::template toSecondChar<BoardX>(to_pos),
                   Dir::toString(dir),
                   (std::uint32_t)dir);
            index++;
        }
        printf("};\n\n");
    }

    template <size_type EmptyColor = Color::Empty, size_type UnknownColor = Color::Unknown>
    static void display_num_move_list(const move_list_t & move_list) {
        size_type index = 0;
        printf("Answer_Move_Path[ %u ] = {\n", (std::uint32_t)move_list.size());
        for (auto iter : move_list) {
            Position from_pos   = iter.from_pos;
            Position to_pos     = iter.to_pos;
            size_type num       = iter.color;
            size_type dir       = iter.dir;
            printf("    [%2u]: [%c], %c%c --> %c%c, dir: %-5s (%u)\n",
                   (std::uint32_t)(index + 1),
                   Number<EmptyColor, UnknownColor>::toChar(num),
                   Position::template toFirstChar<BoardX>(from_pos),
                   Position::template toSecondChar<BoardX>(from_pos),
                   Position::template toFirstChar<BoardX>(to_pos),
                   Position::template toSecondChar<BoardX>(to_pos),
                   Dir::toString(dir),
                   (std::uint32_t)dir);
            index++;
        }
        printf("};\n\n");
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

#pragma pack(pop)

} // namespace AI
} // namespace MagicBlock
