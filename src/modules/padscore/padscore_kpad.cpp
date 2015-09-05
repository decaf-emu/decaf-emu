#include "padscore.h"
#include "padscore_kpad.h"
#include "padscore_wpad.h"

void
KPADInit()
{
   KPADInitEx(0, 0);
}

void
KPADInitEx(uint32_t unk1, uint32_t unk2)
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
}

void
PadScore::registerKPADFunctions()
{
   RegisterKernelFunction(KPADInit);
   RegisterKernelFunction(KPADInitEx);
   RegisterKernelFunction(KPADGetMplsWorkSize);
   RegisterKernelFunction(KPADSetMplsWorkarea);
}
