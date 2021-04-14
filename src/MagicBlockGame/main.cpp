
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlockGame.h"

#include "ErrorCode.h"
#include "CPUWarmUp.h"
#include "StopWatch.h"

using namespace PuzzleGame;

void solve_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("solve_sliding_puzzle()\n\n");

    MagicBlockGame<5, 5, 3, 3> game;
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

    MagicBlockGame<5, 5, 3, 3> game;
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

    MagicBlockGame<5, 5, 3, 3> game;
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

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

    solve_sliding_puzzle();
    solve_sliding_puzzle_queue();

    solve_magic_block_game();

#if !defined(_NDEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
