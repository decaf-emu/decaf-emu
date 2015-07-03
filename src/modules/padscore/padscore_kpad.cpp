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

void
PadScore::registerKPADFunctions()
{
   RegisterKernelFunction(KPADInit);
   RegisterKernelFunction(KPADInitEx);
}
