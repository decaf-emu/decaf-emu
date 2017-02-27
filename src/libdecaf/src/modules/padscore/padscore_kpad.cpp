#include "padscore.h"
#include "padscore_kpad.h"
#include "padscore_wpad.h"

namespace padscore
{

void
KPADInit()
{
   KPADInitEx(0, 0);
}

void
KPADInitEx(uint32_t unk1,
           uint32_t unk2)
{
   WPADInit();
}

uint32_t
KPADGetMplsWorkSize()
{
   return 1024;
}

void
KPADSetMplsWorkarea(char *buffer)
{
   // sizeof(buffer) = KPADGetMplsWorkSize()
   decaf_warn_stub();
}

// Enable "Direct Pointing Device"
void
KPADEnableDPD(uint32_t channel)
{
   decaf_warn_stub();
}

// Disable "Direct Pointing Device"
void
KPADDisableDPD(uint32_t channel)
{
   decaf_warn_stub();
}

void
Module::registerKPADFunctions()
{
   RegisterKernelFunction(KPADInit);
   RegisterKernelFunction(KPADInitEx);
   RegisterKernelFunction(KPADGetMplsWorkSize);
   RegisterKernelFunction(KPADSetMplsWorkarea);
   RegisterKernelFunction(KPADEnableDPD);
   RegisterKernelFunction(KPADDisableDPD);
}

} // namespace padscore
