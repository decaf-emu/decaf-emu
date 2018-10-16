#include "nn_fp.h"
#include "nn_fp_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_fp
{

struct StaticLibData
{
   be2_val<uint32_t> initialiseCount;
};

static virt_ptr<StaticLibData>
sLibData = nullptr;

nn::Result
Initialize()
{
   decaf_warn_stub();
   sLibData->initialiseCount++;
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   if (sLibData->initialiseCount > 0) {
      sLibData->initialiseCount--;
   }
   return nn::ResultSuccess;
}

bool
IsInitialized()
{
   decaf_warn_stub();
   return sLibData->initialiseCount > 0;
}

bool
IsOnline()
{
   decaf_warn_stub();
   return false;
}

nn::Result
GetFriendList(virt_ptr<void> list,
              virt_ptr<uint32_t> outLength,
              uint32_t index,
              uint32_t listSize)
{
   decaf_warn_stub();

   if (outLength) {
      *outLength = 0u;
   }

   return nn::ResultSuccess;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn2fpFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn2fpFv",
                              Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn2fpFv",
                              IsInitialized);
   RegisterFunctionExportName("IsOnline__Q2_2nn2fpFv",
                              IsOnline);
   RegisterFunctionExportName("GetFriendList__Q2_2nn2fpFPUiT1UiT3",
                              GetFriendList);

   RegisterDataInternal(sLibData);
}

} // namespace cafe::nn_fp
