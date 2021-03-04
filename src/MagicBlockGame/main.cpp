
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
    MagicBlockGame<5, 5, 3, 3> game;
    int readStatus = game.readInput("input_test.txt");
    printf("readStatus = %d\n\n", readStatus);

    jtest::StopWatch sw;

    sw.start();
    bool solvable = game.solve_3x3();
    sw.stop();
    double elapsed_time = sw.getElapsedMillisec();

    if (solvable) {
        printf("Has answer!\n\n");
        printf("MinSteps: %d\n\n", (int)game.getSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
        printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
        game.getSteps();
        game.getMoves();
    }
    else {
        printf("No answer!\n\n");
    }

#if defined(_MSC_VER)
    ::system("pause");
#endif
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
        printf("MinSteps: %d\n\n", (int)game.getSteps());
        printf("Map Used: %d\n\n", (int)game.getMapUsed());
        printf("Total elapsed time: %0.3f ms\n\n", elapsed_time);
        game.getSteps();
        game.getMoves();
    }
    else {
        printf("No answer!\n\n");
    }
}

int main(int argc, char * argv[])
{
    jtest::CPU::warmup(1000);

    test_sliding_puzzle();
    test_magic_block_game();

#if !defined(NDEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif
    return 0;
}
