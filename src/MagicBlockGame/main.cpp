
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

    MagicBlock::SparseBitset<Board<5, 5>, 3, 25, 1>::shutdown();
}

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

    SparseTrieBitset_test();

    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();

    solve_magic_block_game();
    solve_magic_block_game_bitmap();

#if !defined(_NDEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
