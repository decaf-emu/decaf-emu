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
AOC_CalculateWorkBufferSize(uint32_t unk)
{
   if (unk > 256) {
      unk = 256;
   }

   return (unk * 0x61) + 0x80;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunctionName("AOC_Initialize", AOC_Initialize);
   RegisterKernelFunctionName("AOC_Finalize", AOC_Finalize);
   RegisterKernelFunctionName("AOC_CalculateWorkBufferSize", AOC_CalculateWorkBufferSize);
}

} // namespace aoc

} // namespace nn
