
#if defined(_MSC_VER)
#if 1
#ifndef NDEBUG
#include <vld.h>
#endif
#endif
#endif // _MSC_VER

#ifndef __SSE2__
#define __SSE2__
#endif

#ifndef __AVX2__
#define __AVX2__
#endif

#define MBG_USE_SSE2    1
#define MBG_USE_AVX2    1

#define DISABLE_CPU_WARM_UP

#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlock/AI/ErrorCode.h"
#include "MagicBlock/AI/SlidingPuzzle/SlidingPuzzle.h"
#include "MagicBlock/AI/SlidingPuzzle/SlidingUnknownPuzzle.h"
#include "MagicBlock/AI/SlidingPuzzle/SlidingUnknownPuzzle3x3.h"
#include "MagicBlock/AI/SlidingPuzzle/SlidingColorPuzzle.h"
#include "MagicBlock/AI/TwoPhase_v1/Game.h"
#include "MagicBlock/AI/TwoPhase_ida/IDAGame.h"
#include "MagicBlock/AI/TwoEndpoint/Game.h"
#include "MagicBlock/AI/Algorithm.h"
#include "MagicBlock/AI/UnitTest.h"

#include "MagicBlock/AI/Console.h"
#include "MagicBlock/AI/CPUWarmUp.h"
#include "MagicBlock/AI/StopWatch.h"

using namespace MagicBlock::AI;

#if defined(_MSC_VER)
#define PUZZLES_BASE_DIR    "..\\..\\..\\puzzles\\"
#else
#define PUZZLES_BASE_DIR    "puzzles/"
#endif

#define PUZZLES_PATH(filename)   PUZZLES_BASE_DIR filename

struct Category {
    enum {
        TwoPhase_v1,
        TwoPhase_v2,
        TwoPhase_IDA,
        TwoEndpoint,
        Last
    };
};

struct SolverId {
    enum {
        Normal,
        // Stand alone mode
        Queue,
        // Stand alone mode
        BitSet,
        // Immediate mode
        BitSetImmediate,
        // std::set<T>
        StdSet,
        // std::unordered_set<T>
        StdHashSet
    };
};

template <std::size_t CategoryId>
static const char * get_category_name()
{
    if (CategoryId == Category::TwoPhase_v1) {
        return "Algorithm::TwoPhase_v1";
    }
    else if (CategoryId == Category::TwoPhase_v2) {
        return "Algorithm::TwoPhase_v2";
    }
    else if (CategoryId == Category::TwoPhase_IDA) {
        return "Algorithm::TwoPhase_IDA";
    }
    else if (CategoryId == Category::TwoEndpoint) {
        return "Algorithm::TwoEndpoint";
    }
    else {
        return "Algorithm::Unkown";
    }
}

template <std::size_t N_SolverId>
static const char * get_solver_name()
{
    if (N_SolverId == SolverId::Queue) {
        return "SolverId::Queue";
    }
    else if (N_SolverId == SolverId::BitSet) {
        return "SolverId::BitSet";
    }
    else if (N_SolverId == SolverId::BitSetImmediate) {
        return "SolverId::BitSetImmediate";
    }
    else if (N_SolverId == SolverId::StdSet) {
        return "SolverId::StdSet";
    }
    else if (N_SolverId == SolverId::StdHashSet) {
        return "SolverId::StdHashSet";
    }
    else {
        return "SolverId::Normal";
    }
}

template <std::size_t N_SolverId, bool SearchAllAnswers = false>
void solve_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle<%s, SearchAllAnswers = %s>()\n\n",
            get_solver_name<N_SolverId>(),
            (SearchAllAnswers ? "true" : "false"));

    SlidingPuzzle<3, 3, SearchAllAnswers> slidingPuzzle;
    int readStatus = slidingPuzzle.readConfig(PUZZLES_PATH("sliding_puzzle.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::Queue) {
        solvable = slidingPuzzle.queue_solve();
    }
    else {
        solvable = slidingPuzzle.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        slidingPuzzle.display_answer_boards();
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)slidingPuzzle.getMinSteps());
        printf("Map Used: %d\n\n", (int)slidingPuzzle.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t N_SolverId, bool SearchAllAnswers = false>
void solve_sliding_unknown_puzzle_3x3()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_unknown_puzzle_3x3<%s, SearchAllAnswers = %s>()\n\n",
            get_solver_name<N_SolverId>(),
            (SearchAllAnswers ? "true" : "false"));

    SlidingUnknownPuzzle3x3<3, 3, 8, 3, SearchAllAnswers> slidingPuzzle;
    int readStatus = slidingPuzzle.readConfig(PUZZLES_PATH("sliding_puzzle_unknown-3x3.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    slidingPuzzle.display_start_boards();

    sw.start();
    if (N_SolverId == SolverId::Queue) {
        solvable = slidingPuzzle.queue_solve();
    }
    else {
        solvable = slidingPuzzle.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        slidingPuzzle.display_answer_boards();
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)slidingPuzzle.getMinSteps());
        printf("Map Used: %d\n\n", (int)slidingPuzzle.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t N_SolverId, bool SearchAllAnswers = false>
void solve_sliding_unknown_puzzle_5x3()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_unknown_puzzle_5x3<%s, SearchAllAnswers = %s>()\n\n",
            get_solver_name<N_SolverId>(),
            (SearchAllAnswers ? "true" : "false"));

    SlidingUnknownPuzzle<5, 3, 6, 3, SearchAllAnswers> slidingPuzzle;
    int readStatus = slidingPuzzle.readConfig(PUZZLES_PATH("sliding_puzzle_unknown-5x3.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    slidingPuzzle.display_start_boards();

    sw.start();
    if (N_SolverId == SolverId::Queue) {
        solvable = slidingPuzzle.queue_solve();
    }
    else {
        solvable = slidingPuzzle.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        slidingPuzzle.display_answer_boards();
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)slidingPuzzle.getMinSteps());
        printf("Map Used: %d\n\n", (int)slidingPuzzle.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t N_SolverId, bool SearchAllAnswers = false>
void solve_sliding_unknown_puzzle_5x5()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_unknown_puzzle_5x5<%s, SearchAllAnswers = %s>()\n\n",
            get_solver_name<N_SolverId>(),
            (SearchAllAnswers ? "true" : "false"));

    SlidingUnknownPuzzle<5, 5, 6, 3, SearchAllAnswers> slidingPuzzle;
    int readStatus = slidingPuzzle.readConfig(PUZZLES_PATH("sliding_puzzle_unknown-5x5.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    slidingPuzzle.display_start_boards();

    sw.start();
    if (N_SolverId == SolverId::Queue) {
        solvable = slidingPuzzle.queue_solve();
    }
    else {
        solvable = slidingPuzzle.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        slidingPuzzle.display_answer_boards();
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)slidingPuzzle.getMinSteps());
        printf("Map Used: %d\n\n", (int)slidingPuzzle.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t N_SolverId, bool AllowRotate = true>
void solve_sliding_color_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_color_puzzle<%s, AllowRotate = %s>()\n\n",
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    SlidingColorPuzzle<3, 3, AllowRotate> slidingPuzzle;
    int readStatus = slidingPuzzle.readConfig(PUZZLES_PATH("sliding_color_puzzle.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::Queue) {
        solvable = slidingPuzzle.queue_solve();
    }
    else {
        solvable = slidingPuzzle.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)slidingPuzzle.getMinSteps());
        printf("Map Used: %d\n\n", (int)slidingPuzzle.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t CategoryId, std::size_t N_SolverId, bool AllowRotate = true>
void solve_magic_block_two_phase_v1()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    TwoPhase_v1::Game<5, 5, 3, 3, AllowRotate> game;

    int readStatus = game.readConfig(PUZZLES_PATH("magic_block.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::BitSet) {
        solvable = game.bitset_solve();
    }
    else if (N_SolverId == SolverId::BitSetImmediate) {
        solvable = game.bitset_solve_immediate();
    }
    else {
        solvable = game.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    if (solvable) {
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t CategoryId, std::size_t N_SolverId, bool AllowRotate = true>
void solve_magic_block_two_phase_ida()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    TwoPhase::IDAGame<5, 5, 3, 3, AllowRotate> game;

    int readStatus = game.readConfig(PUZZLES_PATH("magic_block.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::BitSet) {
        if (AllowRotate)
            solvable = game.bitset_solve(MAX_ROTATE_FORWARD_DEPTH, MAX_ROTATE_BACKWARD_DEPTH);
        else
            solvable = game.bitset_solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    else {
        if (AllowRotate)
            solvable = game.stdset_solve(MAX_ROTATE_FORWARD_DEPTH, MAX_ROTATE_BACKWARD_DEPTH);
        else
            solvable = game.stdset_solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    (void)solvable;
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    if (solvable) {
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t CategoryId, std::size_t N_SolverId, bool AllowRotate = true>
void solve_magic_block_two_endpoint()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    TwoEndpoint::Game<5, 5, 3, 3, AllowRotate> game;

    int readStatus = game.readConfig(PUZZLES_PATH("magic_block.txt"));
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (Error: %s)\n\n", readStatus, ErrorCode::toString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::BitSet) {
        if (AllowRotate)
            solvable = game.bitset_solve(MAX_ROTATE_FORWARD_DEPTH, MAX_ROTATE_BACKWARD_DEPTH);
        else
            solvable = game.bitset_solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    else {
        if (AllowRotate)
            solvable = game.stdset_solve(MAX_ROTATE_FORWARD_DEPTH, MAX_ROTATE_BACKWARD_DEPTH);
        else
            solvable = game.stdset_solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    printf("solve_magic_block<%s, %s, AllowRotate = %s>()\n\n",
            get_category_name<CategoryId>(),
            get_solver_name<N_SolverId>(),
            (AllowRotate ? "true" : "false"));

    if (solvable) {
        printf("Found a answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("Not found a answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

template <std::size_t CategoryId, std::size_t N_SolverId, bool AllowRotate = true>
void solve_magic_block()
{
    if (CategoryId == Category::TwoPhase_v1) {
        solve_magic_block_two_phase_v1<CategoryId, N_SolverId, AllowRotate>();
    }
    else if (CategoryId == Category::TwoPhase_v2) {
        //solve_magic_block_two_phase_v2<CategoryId, N_SolverId, AllowRotate>();
    }
    else if (CategoryId == Category::TwoPhase_IDA) {
        solve_magic_block_two_phase_ida<CategoryId, N_SolverId, AllowRotate>();
    }
    else if (CategoryId == Category::TwoEndpoint) {
        solve_magic_block_two_endpoint<CategoryId, N_SolverId, AllowRotate>();
    }
    else {
        static_assert((CategoryId < Category::Last), "Error: Unknown CategoryId.");
    }
}

int main(int argc, char * argv[])
{
    jtest::cpu::warmUp(1000);

#if 0
    UnitTest();
    //Console::readKeyLast();
    //return 0;
#endif

    if (0) {

#if 0
    solve_sliding_puzzle<SolverId::Normal, true>();
    solve_sliding_puzzle<SolverId::Queue, true>();

    Console::readKeyLine();
#endif

#if 1
    solve_sliding_unknown_puzzle_3x3<SolverId::Normal, true>();
    solve_sliding_unknown_puzzle_3x3<SolverId::Queue, true>();

    Console::readKeyLine();
#endif

#if 1
    solve_sliding_unknown_puzzle_5x3<SolverId::Normal, true>();
    solve_sliding_unknown_puzzle_5x3<SolverId::Queue, true>();

    Console::readKeyLine();
#endif

#if 1
    solve_sliding_unknown_puzzle_5x5<SolverId::Normal, true>();
    solve_sliding_unknown_puzzle_5x5<SolverId::Queue, true>();

    Console::readKeyLine();
#endif

#if 0
    solve_sliding_color_puzzle<SolverId::Normal, true>();
    solve_sliding_color_puzzle<SolverId::Queue, true>();

    solve_sliding_color_puzzle<SolverId::Normal, false>();
    solve_sliding_color_puzzle<SolverId::Queue, false>();

    Console::readKeyLine();
#endif

    }

    if (0) {

    ////////////////////////////////////////////////////////////////////////

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::Normal, true>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::Queue, true>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::BitSet, true>();
    Console::readKeyLine();
#endif

#if 1
    solve_magic_block<Category::TwoPhase_v1, SolverId::BitSetImmediate, true>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_IDA, SolverId::BitSet, true>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoEndpoint, SolverId::StdSet, true>();
    Console::readKeyLine();
#endif

#if 1
    solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, true>();
    Console::readKeyLine();
#endif

    ////////////////////////////////////////////////////////////////////////

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::Normal, false>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::Queue, false>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::BitSet, false>();
    Console::readKeyLine();
#endif

#if 1
    solve_magic_block<Category::TwoPhase_v1, SolverId::BitSetImmediate, false>();
    Console::readKeyLine();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_IDA, SolverId::BitSet, false>();
    Console::readKeyLast();
#endif

#if 0
    solve_magic_block<Category::TwoEndpoint, SolverId::StdSet, false>();
    Console::readKeyLine();
#endif

#if 1
    solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, false>();
    Console::readKeyLast();
#endif

    ////////////////////////////////////////////////////////////////////////

    }

    ////////////////////////////////////////////////////////////////////////

    if (1) {

        //solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, true>();
        //Console::readKeyLine();

        solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, false>();
        Console::readKeyLine();

    }

    ////////////////////////////////////////////////////////////////////////

    return 0;
}
