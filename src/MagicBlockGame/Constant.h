#pragma once

#include <stdlib.h>

#include <stdint.h>
#include <stddef.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace MagicBlock {

static const size_t MaxRotateType = 4;
static const size_t MaxPhrase1Type = 4;

namespace System {

static void pause()
{
#if defined(_MSC_VER)
    ::system("pause");
#endif
}

} // namespace System

} // namespace MagicBlock
