
#if defined(_MSC_VER)
#include <vld.h>
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

struct FuncId {
    enum {
        NormalSolver,
        QueueSolver,
        BitSetSolver,
        StandAloneBitSetSolver,
    };
};

template <std::size_t N_FuncId>
static const char * get_func_name()
{
    if (N_FuncId == FuncId::QueueSolver) {
        return "FuncId::QueueSolver";
    }
    else if (N_FuncId == FuncId::BitSetSolver) {
        return "FuncId::BitSetSolver";
    }
    else if (N_FuncId == FuncId::StandAloneBitSetSolver) {
        return "FuncId::StandAloneBitSetSolver";
    }
    else {
        return "FuncId::NormalSovler";
    }
}

void solve_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle()\n\n");

    MagicBlock::v1::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("sliding_puzzle.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
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

    MagicBlock::v1::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("sliding_puzzle.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
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

template <std::size_t N_FuncId = FuncId::NormalSolver, bool AllowRotate = true>
void solve_magic_block_game_v1()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block_game_v1<%s>()\n\n", get_func_name<N_FuncId>());

    MagicBlock::v1::Game<5, 5, 3, 3, AllowRotate> game;
    int readStatus = game.readInput("magic_block.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    sw.start();
    if (N_FuncId == FuncId::BitSetSolver) {
        solvable = game.bitset_solve();
    }
    else if (N_FuncId == FuncId::StandAloneBitSetSolver) {
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

template <std::size_t N_FuncId = FuncId::NormalSolver, bool AllowRotate = true>
void solve_magic_block_game_two_endpoint()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block_game_two_endpoint<%s>()\n\n", get_func_name<N_FuncId>());

    MagicBlock::TwoEndpoint::Game<5, 5, 3, 3, AllowRotate> game;
    int readStatus = game.readInput("magic_block.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
        return;
    }

    bool solvable;
    jtest::StopWatch sw;

    std::size_t max_forward_depth = MAX_FORWARD_DEPTH;
    std::size_t max_backward_depth = MAX_BACKWARD_DEPTH;

    sw.start();
    if (N_FuncId == FuncId::BitSetSolver) {
        solvable = game.bitset_solve(MAX_FORWARD_DEPTH, MAX_BACKWARD_DEPTH);
    }
    else {
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

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

#ifdef NDEBUG
    UnitTest();
#endif

    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();

#if 0
#ifdef NDEBUG
    solve_magic_block_game<FuncId::NormalSolver, true>();
    System::pause();
#endif // !NDEBUG
#endif

#if 1
    solve_magic_block_game_two_endpoint<FuncId::BitSetSolver, true>();
    System::pause();
#endif

#if 0
#if 1
    solve_magic_block_game_v1<FuncId::BitSetSolver, true>();
#else
    solve_magic_block_game_v1<FuncId::StandAloneBitSetSolver, true>();
#endif
    System::pause();
#endif

    return 0;
}
