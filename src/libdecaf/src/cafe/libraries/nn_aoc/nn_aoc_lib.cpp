#include "nn_aoc.h"
#include "nn_aoc_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_aoc
{

AOCError
AOC_Initialize()
{
   decaf_warn_stub();
   return AOCError::OK;
}

AOCError
AOC_Finalize()
{
   decaf_warn_stub();
   return AOCError::OK;
}

uint32_t
AOC_CalculateWorkBufferSize(uint32_t maxTitles)
{
   if (maxTitles > 256) {
      maxTitles = 256;
   }

   return (0x61 * maxTitles) + 0x80;
}

AOCError
AOC_ListTitle(virt_ptr<uint32_t> outTitleCount,
              virt_ptr<AOCTitle> titles,
              uint32_t maxTitles,
              virt_ptr<void> workBuffer,
              uint32_t workBufferSize)
{
   decaf_warn_stub();
   *outTitleCount = 0u;
   return AOCError::OK;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("AOC_Initialize",
                              AOC_Initialize);
   RegisterFunctionExportName("AOC_Finalize",
                              AOC_Finalize);
   RegisterFunctionExportName("AOC_CalculateWorkBufferSize",
                              AOC_CalculateWorkBufferSize);
   RegisterFunctionExportName("AOC_ListTitle",
                              AOC_ListTitle);
}

} // namespace cafe::nn_aoc
