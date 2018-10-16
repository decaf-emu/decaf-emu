#pragma once
#include "nn_aoc_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_aoc
{

#pragma pack(push, 1)

struct AOCTitle
{
   UNKNOWN(0x61);
};
CHECK_SIZE(AOCTitle, 0x61);

#pragma pack(pop)

AOCError
AOC_Initialize();

AOCError
AOC_Finalize();

uint32_t
AOC_CalculateWorkBufferSize(uint32_t maxTitles);

AOCError
AOC_ListTitle(virt_ptr<uint32_t> outTitleCount,
              virt_ptr<AOCTitle> titles,
              uint32_t maxTitles,
              virt_ptr<void> workBuffer,
              uint32_t workBufferSize);

} // namespace cafe::nn_aoc
