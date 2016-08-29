#pragma once
#include "nn_aoc_enum.h"
#include <common/structsize.h>
#include <common/types.h>

namespace nn
{

namespace aoc
{

#pragma pack(push, 1)

struct AOCTitle
{
   UNKNOWN(0x61);
};
CHECK_SIZE(AOCTitle, 0x61);

#pragma pack(pop)

AOCResult
AOC_Initialize();

AOCResult
AOC_Finalize();

uint32_t
AOC_CalculateWorkBufferSize(uint32_t maxTitles);

AOCResult
AOC_ListTitle(be_val<uint32_t> *titleCount,
              AOCTitle *titles,
              uint32_t maxTitles,
              void *workBuffer,
              uint32_t workBufferSize);

}  // namespace aoc

}  // namespace nn
