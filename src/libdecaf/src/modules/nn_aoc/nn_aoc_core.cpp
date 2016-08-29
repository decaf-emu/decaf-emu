#include "nn_aoc.h"
#include "nn_aoc_core.h"

namespace nn
{

namespace aoc
{

AOCResult
AOC_Initialize()
{
   return AOCResult::Success;
}

AOCResult
AOC_Finalize()
{
   return AOCResult::Success;
}

uint32_t
AOC_CalculateWorkBufferSize(uint32_t maxTitles)
{
   if (maxTitles > 256) {
      maxTitles = 256;
   }

   return (maxTitles * 0x61) + 0x80;
}

AOCResult
AOC_ListTitle(be_val<uint32_t> *titleCount,
              AOCTitle *titles,
              uint32_t maxTitles,
              void *workBuffer,
              uint32_t workBufferSize)
{
   *titleCount = 0;
   return AOCResult::Success;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("AOC_Initialize", AOC_Initialize);
   RegisterKernelFunctionName("AOC_Finalize", AOC_Finalize);
   RegisterKernelFunctionName("AOC_CalculateWorkBufferSize", AOC_CalculateWorkBufferSize);
   RegisterKernelFunctionName("AOC_ListTitle", AOC_ListTitle);
}

} // namespace aoc

} // namespace nn
