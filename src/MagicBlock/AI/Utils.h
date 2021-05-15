#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "MagicBlock/AI/CPUWarmUp.h"

namespace MagicBlock {
namespace AI {

namespace System {

static void pause()
{
#if defined(_MSC_VER)
    ::system("pause");
#endif
}

} // namespace System

} // namespace AI
} // namespace MagicBlock
