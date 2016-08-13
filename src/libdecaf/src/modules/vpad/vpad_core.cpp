#include "vpad.h"
#include "vpad_core.h"

namespace vpad
{

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

int
VPADControlMotor(uint32_t chan, uint8_t *buffer, uint32_t size)
{
   return 0;
}

void
Module::registerCoreFunctions()
{
   RegisterKernelFunction(VPADInit);
   RegisterKernelFunction(VPADSetAccParam);
   RegisterKernelFunction(VPADSetBtnRepeat);
   RegisterKernelFunction(VPADControlMotor);
}

} // namespace vpad
