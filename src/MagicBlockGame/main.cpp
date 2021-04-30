
#if defined(_MSC_VER)
#include <vld.h>
#endif

#ifndef __SSE2__
#define __SSE2__
#endif

#ifndef __AVX2__
//#define __AVX2__
#endif

#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "ErrorCode.h"
#include "Game.h"
#include "Algorithm.h"
#include "UnitTest.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

using namespace MagicBlock;

void solve_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle()\n\n");

    MagicBlock::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("input_test.txt");
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

    MagicBlock::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("input_test.txt");
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

void solve_magic_block_game()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block_game()\n\n");

    MagicBlock::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("input.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
        return;
    }

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.solve();
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

void solve_magic_block_game_bitmap()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_magic_block_game_bitmap()\n\n");

    MagicBlock::Game<5, 5, 3, 3, true> game;
    int readStatus = game.readInput("input.txt");
    printf("readStatus = %d (%s)\n\n", readStatus, ErrorCode::toStatusString(readStatus));
    if (ErrorCode::isFailure(readStatus)) {
        return;
    }

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.bitmap_solve();
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

    UnitTest();

    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();

#if 0
#ifdef NDEBUG
    solve_magic_block_game();
    System::pause();
#endif // !NDEBUG
#endif

    solve_magic_block_game_bitmap();
    System::pause();

    return 0;
}
