
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlockGame.h"

#include "CPUWarmUp.h"
#include "StopWatch.h"

using namespace PuzzleGame;

void test_sliding_puzzle()
{
    printf("-------------------------------------------------------\n\n");
    printf("test_sliding_puzzle()\n\n");

    MagicBlockGame<5, 5, 3, 3> game;
    int readStatus = game.readInput("input_test.txt");
    printf("readStatus = %d\n\n", readStatus);

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.solve_sliding_puzzle();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        printf("Has answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("No answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

void test_sliding_puzzle_queue()
{
    printf("-------------------------------------------------------\n\n");
    printf("test_sliding_puzzle_queue()\n\n");

    MagicBlockGame<5, 5, 3, 3> game;
    int readStatus = game.readInput("input_test.txt");
    printf("readStatus = %d\n\n", readStatus);

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.queue_solve_sliding_puzzle();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        printf("Has answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("No answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

void test_magic_block_game()
{
    MagicBlockGame<5, 5, 3, 3> game;
    int readStatus = game.readInput("input.txt");
    printf("readStatus = %d\n\n", readStatus);

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.solve();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        printf("Has answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getMinSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
    }
    else {
        printf("No answer!\n\n");
    }

    printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
}

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

    //test_sliding_puzzle();
    //test_sliding_puzzle_queue();

    test_magic_block_game();

#if !defined(_NDEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
