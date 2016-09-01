#include "nn_boss.h"
#include "nn_boss_init.h"

namespace nn
{

namespace boss
{

static bool
gInitialised = false;

nn::Result
Initialize()
{
   decaf_warn_stub();

   gInitialised = true;
   return nn::Result::Success;
}

nn::Result
Finalize()
{
   decaf_warn_stub();

   gInitialised = false;
   return nn::Result::Success;
}

bool
IsInitialized()
{
   decaf_warn_stub();

   return gInitialised;
}

int
GetBossState()
{
   decaf_warn_stub();

   return 1;
}

void
Module::registerInitFunctions()
{
   RegisterKernelFunctionName("Initialize__Q2_2nn4bossFv", nn::boss::Initialize);
   RegisterKernelFunctionName("Finalize__Q2_2nn4bossFv", nn::boss::Finalize);
   RegisterKernelFunctionName("IsInitialized__Q2_2nn4bossFv", nn::boss::IsInitialized);
   RegisterKernelFunctionName("GetBossState__Q2_2nn4bossFv", nn::boss::GetBossState);
}

}  // namespace boss

}  // namespace nn
