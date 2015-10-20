#include "vpad.h"
#include "vpad_core.h"

void
VPADInit()
{
}

void
VPADSetAccParam(uint32_t chan, float unk1, float unk2)
{
}

void
VPADSetBtnRepeat(uint32_t chan, float unk1, float unk2)
{
}

void
VPADGetTPCalibratedPoint(uint32_t, uint32_t, uint32_t)
{
}

void
VPad::registerCoreFunctions()
{
   RegisterKernelFunction(VPADInit);
   RegisterKernelFunction(VPADSetAccParam);
   RegisterKernelFunction(VPADSetBtnRepeat);
   RegisterKernelFunction(VPADGetTPCalibratedPoint);
}
