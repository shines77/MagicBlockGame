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
    BaseAnswerGame(const BaseAnswerGame & src)
        : min_steps_(src.min_steps_), map_used_(src.map_used_) {}
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
    typedef typename board_type::move_seq_t     move_seq_t;
    typedef typename board_type::move_list_t    move_list_t;

    typedef Answer<BoardX, BoardY, EmptyPosValue, UnknownPosValue, IsNumberBoard> this_type;

    board_type *    start_board;
    board_type      final_board;
    move_seq_t      move_seq;
    move_list_t     move_list;

private:
    void internal_copy(const this_type & other) {
        this->start_board = other.start_board;
        this->final_board = other.final_board;
        this->move_seq    = other.move_seq;
        this->move_list   = other.move_list;
    }

    void internal_move(this_type && other) noexcept {
        this->start_board = other.start_board;
        this->final_board = other.final_board;
        this->move_seq    = std::move(other.move_seq);
        this->move_list   = std::move(other.move_list);
    }

    void internal_swap(this_type & other) noexcept {
        std::swap(this->start_board, other.start_board);
        this->final_board.swap(other.final_board);
        this->move_seq.swap(other.move_seq);
        std::swap(this->move_list, other.move_list);
    }

public:
    Answer() noexcept : start_board(nullptr) {}

    Answer(board_type * _start_board, const move_seq_t & _move_seq) noexcept
        : start_board(_start_board), move_seq(_move_seq) {}

    Answer(board_type * _start_board, move_seq_t && _move_seq) noexcept
        : start_board(_start_board), move_seq(std::forward<move_seq_t>(_move_seq)) {}

    Answer(board_type * _start_board, const board_type & _final_board, const move_seq_t & _move_seq) noexcept
        : start_board(_start_board), final_board(_final_board), move_seq(_move_seq) {}

    Answer(board_type * _start_board, const board_type & _final_board, move_seq_t && _move_seq) noexcept
        : start_board(_start_board), final_board(_final_board),
          move_seq(std::forward<move_seq_t>(_move_seq)) {}

    Answer(const this_type & src)
        : start_board(src.start_board), final_board(src.final_board),
          move_seq(src.move_seq), move_list(src.move_list) {
    }

    Answer(this_type && src) noexcept : start_board(nullptr) {
        this->internal_move(std::forward<this_type>(src));
    }

    ~Answer() {}

    this_type & operator = (const this_type & rhs) {
        this->copy(rhs);
        return *this;
    }

    this_type & operator = (this_type && rhs) noexcept {
        this->move(std::forward<this_type>(rhs));
        return *this;
    }

    void clear() {
        this->move_list.clear();
    }

    void copy(const this_type & other) noexcept {
        if (&other != this) {
            this->internal_copy(other);
        }
    }

    void move(this_type && other) noexcept {
        if (&other != this) {
            this->internal_move(std::forward<this_type>(other));
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

    static bool translateMoveSeq(const board_type & board, const move_seq_t & move_seq,
                                  move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) {
        return board_type::template translate_move_seq<EmptyPosValue, UnknownPosValue>(board, move_seq, move_list, empty_pos);
    }

    bool translateMoveSeq(const move_seq_t & move_seq, move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) const {
        assert(this->start_board != nullptr);
        return this->start_board->template translate_move_seq<EmptyPosValue, UnknownPosValue>(move_seq, move_list, empty_pos);
    }

    bool translateMoveSeq(const move_seq_t & move_seq, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMoveSeq(move_seq, this->move_list, empty_pos);
    }

    bool translateMoveSeq(const stage_type & target_stage, move_list_t & move_list, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMoveSeq(target_stage.move_seq, move_list, empty_pos);
    }

    bool translateMoveSeq(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        return this->translateMoveSeq(target_stage.move_seq, empty_pos);
    }

    bool translateMoveSeq(Position empty_pos = std::uint8_t(-1)) const {
        return this->translateMoveSeq(this->move_seq, this->move_list, empty_pos);
    }

    static void displayMoveList(const move_list_t & move_list) {
        if (IsNumberBoard)
            board_type::template display_num_move_list<EmptyPosValue, UnknownPosValue>(move_list);
        else
            board_type::display_move_list(move_list);
    }

    static void displayMoveList(const board_type & board, const move_seq_t & move_seq,
                                Position empty_pos = std::uint8_t(-1)) {
        move_list_t move_list;
        if (board_type::translate_move_seq<EmptyPosValue, UnknownPosValue>(board, move_seq, move_list, empty_pos)) {
            if (IsNumberBoard)
                board_type::template display_num_move_list<EmptyPosValue, UnknownPosValue>(move_list);
            else
                board_type::display_move_list(move_list);
        }
    }

    static void displayMoveList(const board_type & board, const stage_type & target_stage,
                                Position empty_pos = std::uint8_t(-1)) {
        this_type::displayMoveList(board, target_stage.move_seq, empty_pos);
    }

    void displayMoveList(const move_seq_t & move_seq, Position empty_pos = std::uint8_t(-1)) {
        if (this->translateMoveSeq(move_seq, this->move_list, empty_pos)) {
            this_type::displayMoveList(this->move_list);
        }
    }

    void displayMoveList(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        this->displayMoveList(target_stage.move_seq, empty_pos);
    }

    void displayMoveList(Position empty_pos = std::uint8_t(-1)) {
        if (this->translateMoveSeq(this->move_seq, this->move_list, empty_pos)) {
            this_type::displayMoveList(this->move_list);
        }
    }

    void displayMoveListOnly() {
        this_type::displayMoveList(this->move_list);
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
    typedef typename answer_type::move_seq_t   move_seq_t;
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

    const move_seq_t & getAnswerMoveSeq() const {
        return this->best_answer_.move_seq;
    }

    void setAnswerStartBoard(board_type * start_board) {
        this->best_answer_.start_board = start_board;
    }

    void setAnswerMoveSeq(const move_seq_t & move_seq) {
        this->best_answer_.move_seq = move_seq;
    }

    void setAnswerMoveSeq(move_seq_t && move_seq) {
        this->best_answer_.move_seq = std::forward<move_seq_t>(move_seq);
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

    void clearAnswer() {
        this->best_answer_.clear();
    }

    bool translateMoveSeq(const move_seq_t & move_seq, move_list_t & move_list,
                          Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMoveSeq(move_seq, move_list, empty_pos);
    }

    bool translateMoveSeq(const move_seq_t & move_seq, Position empty_pos = std::uint8_t(-1)) {
        return this->best_answer_.translateMoveSeq(move_seq, empty_pos);
    }

    bool translateMoveSeq(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        return this->best_answer_.translateMoveSeq(target_stage, empty_pos);
    }

    bool translateMoveSeq(const stage_type & target_stage, move_list_t & move_list,
                          Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMoveSeq(target_stage, move_list, empty_pos);
    }

    bool translateMoveSeq(Position empty_pos = std::uint8_t(-1)) const {
        return this->best_answer_.translateMoveSeq(empty_pos);
    }

    void displayMoveList(const move_seq_t & move_seq, Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMoveList(move_seq, empty_pos);
    }

    void displayMoveList(const stage_type & target_stage, Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMoveList(target_stage, empty_pos);
    }

    void displayMoveList(Position empty_pos = std::uint8_t(-1)) {
        this->best_answer_.displayMoveList(empty_pos);
    }

    void displayMoveListOnly() {
        this->best_answer_.displayMoveListOnly();
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
    typedef typename answer_type::move_seq_t    move_seq_t;
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

    const move_seq_t & getAnswerMoveSeq(size_type index) const {
        assert(index >= 0 && index < best_answer_list_.size());
        return this->best_answer_list_[index].move_seq;
    }

    void setAnswerStartBoard(size_type index, board_type * start_board) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].start_board = start_board;
    }

    void setAnswerMoveSeq(size_type index, const move_seq_t & move_seq) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].move_seq = move_seq;
    }

    void setAnswerMoveSeq(size_type index, move_seq_t && move_seq) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].move_seq = std::forward<move_seq_t>(move_seq);
    }

    void appendAnswer(const answer_type & answer) {
        this->best_answer_list_.push_back(answer);
    }

    void appendAnswer(answer_type && answer) {
        this->best_answer_list_.push_back(std::forward<answer_type>(answer));
    }

    void appendAnswer(board_type * start_board, const board_type & final_board, const move_seq_t & move_seq) {
        answer_type answer(start_board, final_board, move_seq);
        this->best_answer_list_.push_back(std::move(answer));
    }

    void appendAnswer(board_type * start_board, const board_type & final_board, move_seq_t && move_seq) {
        answer_type answer(start_board, final_board, std::forward<move_seq_t>(move_seq));
        this->best_answer_list_.push_back(std::move(answer));
    }

    void clearAllAnswers() {
        this->best_answer_list_.clear();
    }

    bool translateMoveSeq(size_type index, Position empty_pos = std::uint8_t(-1)) {
        assert(index >= 0 && index < best_answer_list_.size());
        return this->best_answer_list_[index].translateMoveSeq(empty_pos);
    }

    void displayMoveList(size_type index, Position empty_pos = std::uint8_t(-1)) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].displayMoveList(empty_pos);
    }

    void displayMoveListOnly(size_type index) {
        assert(index >= 0 && index < best_answer_list_.size());
        this->best_answer_list_[index].displayMoveListOnly();
    }
};

} // namespace AI
} // namespace MagicBlock
