#pragma once

#include <stdint.h>
#include <stddef.h>

#include <cstdint>
#include <cstddef>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define STAGES_USE_EMPLACE_PUSH     1

namespace MagicBlock {
namespace AI {

static const std::size_t MAX_ROTATE_TYPE = 4;
static const std::size_t MAX_PHASE1_TYPE = 4;

#ifdef NDEBUG

static const std::size_t MAX_PHASE2_DEPTH = 26;

// Two-Endpoint algorithm
static const std::size_t MAX_FORWARD_DEPTH = 26;
static const std::size_t MAX_BACKWARD_DEPTH = 24;

static const std::size_t MAX_ROTATE_FORWARD_DEPTH = 24;
static const std::size_t MAX_ROTATE_BACKWARD_DEPTH = 21;

#else

static const std::size_t MAX_PHASE2_DEPTH = 16;

// Two-Endpoint algorithm
static const std::size_t MAX_FORWARD_DEPTH = 16;
static const std::size_t MAX_BACKWARD_DEPTH = 18;

static const std::size_t MAX_ROTATE_FORWARD_DEPTH = 14;
static const std::size_t MAX_ROTATE_BACKWARD_DEPTH = 16;

#endif // NDEBUG

struct SolverType {
    enum {
        Default,
        Full,
        BackwardFull,
        Phase2_456,

        // Two-phase v1
        Phase1_123,
        Phase2_456_789,

        // Two-phase v2
        Phase1_123_4,
        Phase2_56_789,

        Phase2_Compact
    };
};

} // namespace AI
} // namespace MagicBlock
