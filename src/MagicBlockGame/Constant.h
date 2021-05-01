#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace MagicBlock {

static const std::size_t MAX_ROTATE_TYPE = 4;
static const std::size_t MAX_PHASE1_TYPE = 4;

static const std::size_t MAX_PHASE2_DEPTH = 35;

struct PhaseType {
    enum type {
        Unknown,
        Phase1_1,
        Phase1_12,
        Phase1_123,
        Phase2
    };
};

} // namespace MagicBlock
