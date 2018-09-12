#include "snd_user.h"
#include "snd_user_fx.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snd_user
{

static AXFXAllocFuncPtr
sAXFXMemAlloc = nullptr;

static AXFXFreeFuncPtr
sAXFXMemFree = nullptr;

int32_t
AXFXReverbHiExpGetMemSize(virt_ptr<AXFXReverbHi> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXDelayGetMemSize(virt_ptr<AXFXDelay> chorus)
{
   decaf_warn_stub();
   return 32;
}

int32_t
AXFXDelayExpGetMemSize(virt_ptr<AXFXDelay> chorus)
{
   decaf_warn_stub();
   return 32;
}

void
Library::registerFxSymbols()
{
   RegisterFunctionExport(AXFXReverbHiExpGetMemSize);
   RegisterFunctionExport(AXFXDelayGetMemSize);
   RegisterFunctionExport(AXFXDelayExpGetMemSize);
}

} // namespace cafe::snd_user
