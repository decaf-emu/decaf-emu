#include "cafe/libraries/nn_ec/nn_ec.h"
#include "cafe/libraries/nn_ec/nn_ec_lib.h"
#include "cafe/libraries/nn_ec/nn_ec_memorymanager.h"
#include "cafe/libraries/nn_ec/nn_ec_result.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_ec
{

nn::Result
Initialize(uint32_t unk0)
{
   decaf_warn_stub();
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   return nn::ResultSuccess;
}

nn::Result
SetAllocator(MemoryManager::AllocFn allocFn,
             MemoryManager::FreeFn freeFn)
{
   auto test = nn::Result(0xC1603C80);
   if (!allocFn || !freeFn) {
      return ResultInvalidArgument;
   }

   internal::MemoryManager_SetAllocator(allocFn, freeFn);
   return nn::ResultSuccess;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn2ecFUi",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn2ecFv",
                              Finalize);
   RegisterFunctionExportName("SetAllocator__Q2_2nn2ecFPFUii_PvPFPv_v",
                              SetAllocator);
}

} // namespace cafe::nn_ec
