#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <fstream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>    // For std::swap(), until C++11
#include <utility>      // For std::swap(), since C++11
#include <climits>      // For std::numeric_limits<T>
#include <type_traits>  // For std::forward<T>

#include "MagicBlock/AI/Constant.h"
#include "MagicBlock/AI/Color.h"
#include "MagicBlock/AI/Move.h"
#include "MagicBlock/AI/Board.h"

namespace MagicBlock {
namespace AI {

class BaseAnswerGame
{
public:
    typedef std::size_t size_type;

protected:
    size_type min_steps_;
    size_type map_used_;

public:
    BaseAnswerGame() : min_steps_(std::numeric_limits<size_type>::max()), map_used_(0) {}
    ~BaseAnswerGame() {}

    size_type getMinSteps() const {
        return this->min_steps_;
    }

    size_type getMapUsed() const {
        return this->map_used_;
    }

    void setMinSteps(size_type min_steps) {
        this->min_steps_ = min_steps;
    }

    void setMapUsed(size_type map_used) {
        this->map_used_ = map_used;
    }

    bool isMinSteps(size_type steps) const {
        return (steps < this->min_steps_);
    }

    bool isEqualMinSteps(size_type steps) const {
        return (steps == this->min_steps_);
    }
};

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t EmptyPosValue = Color::Empty,
          std::size_t UnknownPosValue = Color::Unknown,
          bool IsNumberBoard = false>
class Answer {
public:
    typedef Board<BoardX, BoardY>               board_type;
    typedef Stage<BoardX, BoardY>               stage_type;
    typedef typename board_type::size_type      size_type;
    typedef typename board_type::move_path_t    move_path_t;
    typedef typename board_type::move_list_t    move_list_t;

    typedef Answer<BoardX, BoardY, EmptyPosValue, UnknownPosValue, IsNumberBoard> this_type;

    board_type *    start_board;
    board_type      final_board;
    move_path_t     move_path;
    move_list_t     move_list;

private:
    void internal_copy(const this_type & other) {
        this->start_board = other.start_board;
        this->final_board = other.final_board;
        this->move_path   = other.move_path;
        this->move_list   = other.move_list;
    }

    void internal_swap(this_type & other) noexcept {
        std::swap(this->start_board, other.start_board);
        this->final_board.swap(other.final_board);
        std::swap(this->move_path, other.move_path);
        std::swap(this->move_list, other.move_list);
    }

public:
    Answer() noexcept : start_board(nullptr) {}

    Answer(board_type * _start_board, const move_path_t & _move_path) noexcept
        : start_board(_start_board), move_path(_move_path) {}

    Answer(board_type * _start_board, move_path_t && _move_path) noexcept
        : start_board(_start_board), move_path(std::forward<move_path_t>(_move_path)) {}

    Answer(board_type * _start_board, const move_path_t & _move_path, const board_type & _final_board) noexcept
        : start_board(_start_board), final_board(_final_board), move_path(_move_path) {}

    Answer(board_type * _start_board, move_path_t && _move_path, board_type && _final_board) noexcept
        : start_board(_start_board), final_board(std::forward<board_type>(_final_board)),
          move_path(std::forward<move_path_t>(_move_path)) {}

    Answer(const this_type & src)
        : start_board(src.start_board), final_board(src.final_board),
          move_path(src.move_path), move_list(src.move_list) {
    }

    Answer(this_type && src) noexcept : start_board(nullptr) {
        this->internal_swap(src);
    }

    ~Answer() = default;

    this_type & operator = (const this_type & rhs) {
        this->copy(rhs);
        return *this;
    }

    this_type & operator = (this_type && rhs) noexcept {
        this->swap(rhs);
        return *this;
    }

    void copy(const this_type & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void swap(this_type & other) noexcept {
        if (&other != this) {
            this->internal_swap(other);
        }
    }

    board_type * getStartBoard() const {
        return this->start_board;
    }

    void setStartBoard(board_type * start_board) {
        this->start_board = start_board;
    }

    static bool translateMovePath(const board_type & board, const move_path_t & move_path,
                                  move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) {
        return board_type::template translate_move_path<EmptyPosValue, UnknownPosValue>(board, move_path, move_list, empty_pos);
    }

    bool translateMovePath(const move_path_t & move_path, move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) const {
        assert(this->start_board != nullptr);
        return this->start_board->template translate_move_path<EmptyPosValue, UnknownPosValue>(move_path, move_list, empty_pos);
    }

    bool translateMovePath(const move_path_t & move_path, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMovePath(move_path, this->move_list, empty_pos);
    }

    bool translateMovePath(const stage_type & target_stage, move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMovePath(target_stage.move_path, move_list, empty_pos);
    }

    bool translateMovePath(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMovePath(target_stage.move_path, empty_pos);
    }

    bool translateMovePath(Position empty_pos = std::uint8_t(-1)) const {
        return this->translateMovePath(this->move_path, this->move_list, empty_pos);
    }

    static void displayMovePath(const move_list_t & move_list) {
        if (IsNumberBoard)
            board_type::template display_num_move_path<EmptyPosValue, UnknownPosValue>(move_list);
        else
            board_type::display_move_path(move_list);
    }

    static void displayMovePath(const board_type & board, const move_path_t & move_path,
                                Position empty_pos = std::uint8_t(-1)) {
        move_list_t move_list;
        if (board_type::translate_move_path<EmptyPosValue, UnknownPosValue>(board, move_path, move_list, empty_pos)) {
            if (IsNumberBoard)
                board_type::template display_num_move_path<EmptyPosValue, UnknownPosValue>(move_list);
            else
                board_type::display_move_path(move_list);
        }
    }

    static void displayMovePath(const board_type & board, const stage_type & target_stage,
                                Position empty_pos = std::uint8_t(-1)) {
        this_type::displayMovePath(board, target_stage.move_path, empty_pos);
    }

    void displayMovePath(const move_path_t & move_path, Position empty_pos = std::uint8_t(-1)) {
        if (this->translateMovePath(move_path, this->move_list, empty_pos)) {
            this_type::displayMovePath(this->move_list);
        }
    }

    void displayMovePath(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        this->displayMovePath(target_stage.move_path, empty_pos);
    }

    void displayMovePath(Position empty_pos = std::uint8_t(-1)) {
        if (this->translateMovePath(this->move_path, this->move_list, empty_pos)) {
            this_type::displayMovePath(this->move_list);
        }
    }

    void displayMovePathOnly() {
        this_type::displayMovePath(this->move_list);
    }
};

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t EmptyPosValue = Color::Empty,
          std::size_t UnknownPosValue = Color::Unknown,
          bool IsNumberBoard = false>
class SingleAnswerGame : public BaseAnswerGame
{
public:
    typedef Answer<BoardX, BoardY, EmptyPosValue, UnknownPosValue, IsNumberBoard> answer_type;

    typedef typename answer_type::size_type     size_type;
    typedef typename answer_type::board_type    board_type;
    typedef typename answer_type::stage_type    stage_type;
    typedef typename answer_type::move_path_t   move_path_t;
    typedef typename answer_type::move_list_t   move_list_t;

protected:
    answer_type best_answer_;

public:
    SingleAnswerGame() noexcept {};
    ~SingleAnswerGame() {};

    answer_type & getAnswer() {
        return this->best_answer_;
    }

    const answer_type & getAnswer() const {
        return this->best_answer_;
    }

    size_type getAnswerCount() const {
        return 1;
    }

    void setAnswer(const answer_type & answer) {
        this->best_answer_ = answer;
    }

    void setAnswer(answer_type && answer) {
        this->best_answer_ = std::forward<answer_type>(answer);
    }

    board_type * getAnswerStartBoard() const {
        return this->best_answer_.start_board;
    }

    const move_path_t & getAnswerMovePath() const {
        return this->best_answer_.move_path;
    }

    void setAnswerStartBoard(board_type * start_board) {
        this->best_answer_.start_board = start_board;
    }

    void setAnswerMovePath(const move_path_t & move_path) {
        this->best_answer_.move_path = move_path;
    }

    void setAnswerMovePath(move_path_t && move_path) {
        this->best_answer_.move_path = std::forward<move_path_t>(move_path);
    }

    const move_list_t & getAnswerMoveList() const {
        return this->best_answer_.move_list;
    }

    void setAnswerMoveList(const move_list_t & move_list) {
        this->best_answer_.move_list = move_list;
    }

    void setAnswerMoveList(move_list_t && move_list) {
        this->best_answer_.move_list = std::forward<move_list_t>(move_list);
    }

    bool translateMovePath(const move_path_t & move_path, move_list_t & move_list,
                           Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMovePath(move_path, move_list, empty_pos);
    }

    bool translateMovePath(const move_path_t & move_path, Position empty_pos = std::uint8_t(-1)) {
        return this->best_answer_.translateMovePath(move_path, empty_pos);
    }

    bool translateMovePath(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        return this->best_answer_.translateMovePath(target_stage, empty_pos);
    }

    bool translateMovePath(const stage_type & target_stage, move_list_t & move_list,
                           Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMovePath(target_stage, move_list, empty_pos);
    }

    bool translateMovePath(Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMovePath(empty_pos);
    }

    void displayAnswerMoves(const move_path_t & move_path, Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMovePath(move_path, empty_pos);
    }

    void displayAnswerMoves(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMovePath(target_stage, empty_pos);
    }

    void displayAnswerMoves(Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMovePath(empty_pos);
    }

    void displayAnswerMovesOnly() {
        this->best_answer_.displayMovePathOnly();
    }
};

template <std::size_t BoardX, std::size_t BoardY,
          std::size_t EmptyPosValue = Color::Empty,
          std::size_t UnknownPosValue = Color::Unknown,
          bool IsNumberBoard = false>
class MultiAnswerGame : public BaseAnswerGame
{
public:
    typedef Answer<BoardX, BoardY, EmptyPosValue, UnknownPosValue, IsNumberBoard> answer_type;

    typedef typename answer_type::size_type     size_type;
    typedef typename answer_type::board_type    board_type;
    typedef typename answer_type::stage_type    stage_type;
    typedef typename answer_type::move_path_t   move_path_t;
    typedef typename answer_type::move_list_t   move_list_t;

protected:
    std::vector<answer_type> best_answer_list_;

public:
    MultiAnswerGame() noexcept {};
    ~MultiAnswerGame() {};

    std::vector<answer_type> & getAnswerList() {
        return this->best_answer_list_;
    }

    const std::vector<answer_type> & getAnswerList() const {
        return this->best_answer_list_;
    }

    size_type getAnswerCount() const {
        return this->best_answer_list_.size();
    }

    void setAnswer(size_type index, const answer_type & answer) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index] = answer;
    }

    void setAnswer(size_type index, answer_type && answer) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index] = std::forward<answer_type>(answer);
    }

    board_type * getAnswerStartBoard(size_type index) const {
        assert(index >= 0 && index < best_answer_list_.size());
        return this->best_answer_[index].start_board;
    }

    const move_path_t & getAnswerMovePath(size_type index) const {
        assert(index >= 0 && index < best_answer_list_.size());
        return this->best_answer_list_[index].move_path;
    }

    void setAnswerStartBoard(size_type index, board_type * start_board) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].start_board = start_board;
    }

    void setAnswerMovePath(size_type index, const move_path_t & move_path) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].move_path = move_path;
    }

    void setAnswerMovePath(size_type index, move_path_t && move_path) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].move_path = std::forward<move_path_t>(move_path);
    }

    void appendAnswer(const answer_type & answer) {
        this->best_answer_list_.push_back(answer);
    }

    void appendAnswer(answer_type && answer) {
        this->best_answer_list_.push_back(std::forward<answer_type>(answer));
    }

    void appendAnswer(board_type * start_board, const move_path_t & move_path, const board_type & final_board) {
        answer_type answer(start_board, move_path, final_board);
        this->best_answer_list_.push_back(std::move(answer));
    }

    void appendAnswer(board_type * start_board, move_path_t && move_path, board_type && final_board) {
        answer_type answer(start_board, std::forward<move_path_t>(move_path), std::forward<board_type>(final_board));
        this->best_answer_list_.push_back(std::move(answer));
    }

    void clearAllAnswers() {
        this->best_answer_list_.clear();
    }

    bool translateMovePath(size_type index, Position empty_pos = std::uint8_t(-1)) {
        assert(index >= 0 && index < best_answer_list_.size());
        return this->best_answer_list_[index].translateMovePath(empty_pos);
    }

    void displayAnswerMoves(size_type index, Position empty_pos = std::uint8_t(-1)) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].displayMovePath(empty_pos);
    }

    void displayAnswerMovesOnly(size_type index) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].displayMovePathOnly();
    }

    void translateAllMovePath(std::vector<Position> empty_pos_list) {
        size_type empty_size = empty_pos_list.size();
        size_type list_size = this->best_answer_list_.size();
        for (size_type i = 0; i < list_size; i++) {
            if (i >= empty_size)
                this->best_answer_list_[i].translateMovePath(std::uint8_t(-1));
            else
                this->best_answer_list_[i].translateMovePath(empty_pos_list[i]);
        }
    }

    void displayAllAnswerMoves(std::vector<Position> empty_pos_list) {
        size_type empty_size = empty_pos_list.size();
        size_type list_size = this->best_answer_list_.size();
        for (size_type i = 0; i < list_size; i++) {
            if (i >= empty_size)
                this->best_answer_list_[i].displayMovePath(std::uint8_t(-1));
            else
                this->best_answer_list_[i].displayMovePath(empty_pos_list[i]);
        }
    }

    void displayAllAnswerMovesOnly() {
        size_type list_size = this->best_answer_list_.size();
        for (size_type i = 0; i < list_size; i++) {
            this->best_answer_list_[i].displayMovePathOnly();
        }
    }
};

} // namespace AI
} // namespace MagicBlock
