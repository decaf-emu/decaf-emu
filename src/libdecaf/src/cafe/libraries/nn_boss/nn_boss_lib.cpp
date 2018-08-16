#include "nn_boss.h"
#include "nn_boss_lib.h"
#include "nn_boss_result.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::boss
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
   return Success;
}

nn::Result
Finalize()
{
   decaf_warn_stub();
   if (sLibData->initialiseCount > 0) {
      sLibData->initialiseCount--;
   }

   return Success;
}

bool
IsInitialized()
{
   decaf_warn_stub();
   return (sLibData->initialiseCount > 0);
}

BossState
GetBossState()
{
   decaf_warn_stub();
   return static_cast<BossState>(0);
}

void
Library::registerLibSymbols()
{
   RegisterFunctionExportName("Initialize__Q2_2nn4bossFv",
                              Initialize);
   RegisterFunctionExportName("Finalize__Q2_2nn4bossFv",
                              Finalize);
   RegisterFunctionExportName("IsInitialized__Q2_2nn4bossFv",
                              IsInitialized);
   RegisterFunctionExportName("GetBossState__Q2_2nn4bossFv",
                              GetBossState);

   RegisterDataInternal(sLibData);
}

}  // namespace namespace cafe::nn::boss
