#include "nn_ndm.h"
#include "nn_ndm_ndm.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn_ndm
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
   return nn::ResultSuccess;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   if (sNdmData->initialiseCount <= 0) {
      // Panic
      return nn::ResultSuccess;
   }

   sNdmData->initialiseCount--;
   return nn::ResultSuccess;
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
   return nn::ResultSuccess;
}

nn::Result
GetDaemonStatus(virt_ptr<uint32_t> status, // nn::ndm::IDaemon::Status *
                uint32_t unknown) // nn::ndm::Cafe::DaemonName
{
   decaf_warn_stub();
   *status = 3u;
   return nn::ResultSuccess;
}

void
Library::registerNdmSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3ndmFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3ndmFv",
                              Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn3ndmFv",
                              IsInitialized);
   RegisterFunctionExportName("EnableResumeDaemons__Q2_2nn3ndmFv",
                              EnableResumeDaemons);
   RegisterFunctionExportName("GetDaemonStatus__Q2_2nn3ndmFPQ4_2nn3ndm7IDaemon6StatusQ4_2nn3ndm4Cafe10DaemonName",
                              GetDaemonStatus);

   RegisterFunctionExportName("NDMInitialize", Initialize);
   RegisterFunctionExportName("NDMFinalize", Finalize);

   RegisterDataInternal(sNdmData);
}

} // namespace cafe::nn_ndm
