
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "Game.h"

#include "ErrorCode.h"
#include "CPUWarmUp.h"
#include "StopWatch.h"

#include "SparseBitset.h"
#include "jm_malloc.h"

using namespace MagicBlock;
using namespace jm_malloc;

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

void SparseTrieBitset_test()
{
    MagicBlock::SparseBitset<Board<5, 5>, 3, 25, 1> visited;
    Board<5, 5> board;
    board.cells[0] = Color::Blue;
    board.cells[1] = Color::Red;
    board.cells[2] = Color::Green;
    board.cells[3] = Color::Orange;
    board.cells[4] = Color::White;

    board.cells[5] = Color::Yellow;
    board.cells[6] = Color::Empty;
    board.cells[7] = Color::Red;
    board.cells[8] = Color::Blue;
    board.cells[9] = Color::Unknown;
    visited.append(board);

    visited.shutdown();
}

void jm_mallc_SizeClass_test()
{
    ThreadMalloc<0>::SizeClass sizeClass;

    std::size_t index, index1, index2;
    index = sizeClass.sizeToIndex(0);
    assert(index == 0);

    index  = sizeClass.sizeToIndex(4095);
    index1 = sizeClass.sizeToIndex(4096);
    index2 = sizeClass.sizeToIndex(4097);
    assert(index == 92);
    assert(index1 == 92);
    assert(index2 == (index1 + 1));

    index  = sizeClass.sizeToIndex(16383);
    index1 = sizeClass.sizeToIndex(16384);
    index2 = sizeClass.sizeToIndex(16385);
    assert(index == (92 + 96));
    assert(index1 == (92 + 96));
    assert(index2 == (index1 + 1));
}

void jm_mallc_ThreadMalloc_test()
{
    ThreadMalloc<0> threadMalloc;
    threadMalloc.getInstance();

    threadMalloc.shutdown();
}

void jm_mallc_test()
{
    jm_mallc_SizeClass_test();
    jm_mallc_ThreadMalloc_test();
}

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

    jm_mallc_test();
    //SparseTrieBitset_test();

    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();

    solve_magic_block_game();
    //solve_magic_block_game_bitmap();

#if !defined(_NDEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
