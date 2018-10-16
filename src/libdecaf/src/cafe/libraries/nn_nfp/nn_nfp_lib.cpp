#include "nn_nfp.h"
#include "nn_nfp_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/nfp/nn_nfp_result.h"

using namespace nn::nfp;

namespace cafe::nn_nfp
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
   return ResultSuccess;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   if (sLibData->initialiseCount > 0) {
      sLibData->initialiseCount--;
   }
   return ResultSuccess;
}

nn::Result
GetAmiiboSettingsArgs(virt_ptr<AmiiboSettingsArgs> args)
{
   decaf_warn_stub();
   std::memset(args.get(), 0, sizeof(AmiiboSettingsArgs));
   return ResultSuccess;
}

State
GetNfpState()
{
   decaf_warn_stub();
   return State::Uninitialised;
}

nn::Result
SetActivateEvent(uint32_t a1)
{
   decaf_warn_stub();
   return ResultSuccess;
}

nn::Result
SetDeactivateEvent(uint32_t a1)
{
   decaf_warn_stub();
   return ResultSuccess;
}

nn::Result
StartDetection()
{
   decaf_warn_stub();
   return ResultSuccess;
}

nn::Result
StopDetection()
{
   decaf_warn_stub();
   return ResultSuccess;
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn3nfpFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn3nfpFv",
                              Finalize);
   RegisterFunctionExportName("GetAmiiboSettingsArgs__Q2_2nn3nfpFPQ3_2nn3nfp18AmiiboSettingsArgs",
                              GetAmiiboSettingsArgs);
   RegisterFunctionExportName("GetNfpState__Q2_2nn3nfpFv",
                              GetNfpState);
   RegisterFunctionExportName("SetActivateEvent__Q2_2nn3nfpFP7OSEvent",
                              SetActivateEvent);
   RegisterFunctionExportName("SetDeactivateEvent__Q2_2nn3nfpFP7OSEvent",
                              SetDeactivateEvent);
   RegisterFunctionExportName("StartDetection__Q2_2nn3nfpFv",
                              StartDetection);
   RegisterFunctionExportName("StopDetection__Q2_2nn3nfpFv",
                              StopDetection);

   RegisterDataInternal(sLibData);
}

}  // namespace cafe::nn_nfp
