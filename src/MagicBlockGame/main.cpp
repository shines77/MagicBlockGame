
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlockGame.h"

int main(int argc, char * argv[])
{
    MagicBlockGame<5, 5, 3, 3> game;
    int readStatus = game.readInput("input.txt");
    printf("readStatus = %d\n\n", readStatus);

    bool canSolve = game.solve();
    if (canSolve) {
        game.getSteps();
        game.getMoves();
    }

    ::system("pause");
    return 0;
}
