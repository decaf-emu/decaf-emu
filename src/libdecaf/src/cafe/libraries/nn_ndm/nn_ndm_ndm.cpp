#include "nn_ndm.h"
#include "nn_ndm_ndm.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::ndm
{

struct StaticNdmData
{
   be2_val<uint32_t> initialiseCount;
};

static virt_ptr<StaticNdmData>
sNdmData = nullptr;

nn::Result
Initialize()
{
   decaf_warn_stub();
   sNdmData->initialiseCount++;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   if (sNdmData->initialiseCount <= 0) {
      // Panic
      return nn::Result::Success;
   }

   sNdmData->initialiseCount--;
   return nn::Result::Success;
}

bool
IsInitialized()
{
   decaf_warn_stub();
   return sNdmData->initialiseCount > 0;
}

nn::Result
EnableResumeDaemons()
{
   decaf_warn_stub();
   return nn::Result::Success;
}

void
Library::registerNdmSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3ndmFv", Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3ndmFv", Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn3ndmFv", IsInitialized);
   RegisterFunctionExportName("EnableResumeDaemons__Q2_2nn3ndmFv", EnableResumeDaemons);

   RegisterFunctionExportName("NDMInitialize", Initialize);
   RegisterFunctionExportName("NDMFinalize", Finalize);

   RegisterDataInternal(sNdmData);
}

} // namespace cafe::nn::ndm
