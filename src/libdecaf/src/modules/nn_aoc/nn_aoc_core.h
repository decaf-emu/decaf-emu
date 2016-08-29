#pragma once
#include "common/types.h"
#include "nn_aoc_enum.h"

namespace nn
{

namespace aoc
{

AOCResult
AOC_Initialize();

AOCResult
AOC_Finalize();

uint32_t
AOC_CalculateWorkBufferSize(uint32_t unk);

}  // namespace aoc

}  // namespace nn
