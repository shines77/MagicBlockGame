
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>

#include "MagicBlockGame.h"

int main(int argc, char * argv[])
{
    MagicBlockGame game;
    int readStatus = game.readInput("input.txt");
    printf("readStatus = %d\n\n", readStatus);

    int steps = game.solve();

    ::system("pause");
    return 0;
}
