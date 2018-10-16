#include "nn_boss.h"
#include "nn_boss_lib.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
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

}  // namespace namespace cafe::nn_boss
