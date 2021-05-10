#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>

#ifndef NOMINMAX
#define NOMINMAX
#endif

namespace MagicBlock {
namespace AI {

static const std::size_t MAX_ROTATE_TYPE = 4;
static const std::size_t MAX_PHASE1_TYPE = 4;

static const std::size_t MAX_PHASE2_DEPTH = 35;

// Two-Endpoint algorithm
static const std::size_t MAX_FORWARD_DEPTH = 28;
static const std::size_t MAX_BACKWARD_DEPTH = 24;

static const std::size_t MAX_ROTATE_FORWARD_DEPTH = 24;
static const std::size_t MAX_ROTATE_BACKWARD_DEPTH = 20;

struct SolverType {
    enum {
        Default,
        Full,
        BackwardFull,
        Phase1_1,
        Phase1_12,
        Phase1_123,
        Phase2_456,
        Phase2,
        Phase2_Compact
    };
};

} // namespace AI
} // namespace MagicBlock
