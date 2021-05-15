#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "MagicBlock/AI/get_char.h"
#include "MagicBlock/AI/CPUWarmUp.h"

namespace MagicBlock {
namespace AI {
namespace Console {

static int readKey(bool displayTips = true, bool echoInput = false,
                   bool enabledCpuWarmup = true,
                   unsigned int delayMillsecs = 1000)
{
    int keyCode;
    if (displayTips) {
        printf("Press any key to continue ...");
    }

    keyCode = jimi_getch();
    if (echoInput) {
        if (keyCode != EOF)
            printf("%c", (char)keyCode);
        else
            printf("EOF: (%d)", keyCode);
    }

    if (enabledCpuWarmup) {
        printf("\n\n");
    }
    else if (displayTips) {
        printf("\n");
    }

    // After the pause, warm-up and wake-up the CPU for at least 1000 ms.
    if (enabledCpuWarmup) {
        jtest::cpu::warmUp(delayMillsecs);
    }
    return keyCode;
}

static int readKeyLine(int newLines = 2, bool displayTips = true, bool echoInput = false,
                       bool enabledCpuWarmup = true, unsigned int delayMillsecs = 1000)
{
    int keyCode = Console::readKey(displayTips, echoInput, false, 0);
    if (displayTips)
        newLines--;
    if (newLines == 1)
        printf("\n");
    else if (newLines >= 2)
        printf("\n\n");

    // After the pause, warm-up and wake-up the CPU for at least 1000 ms.
    if (enabledCpuWarmup) {
        jtest::cpu::warmUp(delayMillsecs);
    }
    return keyCode;
}

static int readKeyLast(bool displayTips = true, bool echoInput = false)
{
    int keyCode = Console::readKey(displayTips, echoInput, false, 0);
    printf("\n");
    return keyCode;
}

} // namespace Console
} // namespace AI
} // namespace MagicBlock
