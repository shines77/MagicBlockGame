
#if defined(_MSC_VER)
#if 0
#ifndef NDEBUG
#include <vld.h>
#endif
#endif
#endif

#ifndef __SSE2__
#define __SSE2__
#endif

#ifndef __AVX2__
#define __AVX2__
#endif

#define _USE_SSE2_  1
#define _USE_AVX2_  1

#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "ErrorCode.h"
#include "v1/Game.h"
#include "TwoEndpoint/Game.h"
#include "Algorithm.h"
#include "UnitTest.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

using namespace MagicBlock;

struct Category {
    enum {
        TwoPhase_v1,
        TwoPhase_v2,
        TwoEndpoint,
        Last
    };
};

struct SolverId {
    enum {
        Normal,
        Queue,
        BitSet,
        StandAloneBitSet,
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
    else if (N_SolverId == SolverId::StandAloneBitSet) {
        return "SolverId::StandAloneBitSet";
    }
    else {
        return "SolverId::Normal";
    }
}

void solve_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle()\n\n");

    MagicBlock::v1::Game<5, 5, 3, 3, false> game;
    int readStatus = game.readInput("sliding_puzzle.txt");
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
        return;
    }

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.solve_sliding_puzzle();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

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

void solve_sliding_puzzle_queue()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle_queue()\n\n");

    MagicBlock::v1::Game<5, 5, 3, 3, false> game;
    int readStatus = game.readInput("sliding_puzzle.txt");
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
        return;
    }

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.queue_solve_sliding_puzzle();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

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
void solve_magic_block_two_phase()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block<%s, %s>()\n\n", get_category_name<CategoryId>(),
                                              get_solver_name<N_SolverId>());

    MagicBlock::v1::Game<5, 5, 3, 3, AllowRotate> game;

    int readStatus = game.readInput("magic_block.txt");
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_SolverId == SolverId::BitSet) {
        solvable = game.bitset_solve();
    }
    else if (N_SolverId == SolverId::StandAloneBitSet) {
        solvable = game.stand_alone_bitset_solve();
    }
    else {
        solvable = game.solve();
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

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
    printf("solve_magic_block<%s, %s>()\n\n", get_category_name<CategoryId>(),
                                              get_solver_name<N_SolverId>());

    MagicBlock::TwoEndpoint::Game<5, 5, 3, 3, AllowRotate> game;

    int readStatus = game.readInput("magic_block.txt");
    if (ErrorCode::isFailure(readStatus)) {
        printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
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
            solvable = game.solve(MAX_ROTATE_FORWARD_DEPTH, MAX_ROTATE_BACKWARD_DEPTH);
        else
            solvable = game.solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

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
        solve_magic_block_two_phase<CategoryId, N_SolverId, AllowRotate>();
    }
    else if (CategoryId == Category::TwoPhase_v2) {
        //solve_magic_block_two_phase<CategoryId, N_SolverId, AllowRotate>();
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
    jtest::CPU::warmup(1000);

#ifdef NDEBUG
    UnitTest();
#endif

#if 1
    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();
#endif

#if 0
#ifdef NDEBUG
    solve_magic_block<Category::TwoPhase_v1, SolverId::Normal, true>();
    System::pause();
#endif // !NDEBUG
#endif

#if 1
    solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, true>();
    System::pause();
#endif

#if 1
    solve_magic_block<Category::TwoEndpoint, SolverId::BitSet, false>();
    System::pause();
#endif

#if 1
    solve_magic_block<Category::TwoPhase_v1, SolverId::BitSet, true>();
    System::pause();
#endif

#if 0
    solve_magic_block<Category::TwoPhase_v1, SolverId::StandAloneBitSet, false>();
    System::pause();
#endif

    return 0;
}
