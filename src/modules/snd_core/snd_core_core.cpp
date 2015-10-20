#include "snd_core.h"
#include "utils/wfunc_ptr.h"

void
AXInit()
{
   // TODO: AXInit
}

void AXInitProfile(void*, uint32_t)
{
   // TODO: AXInitProfile
}

uint32_t
AXSetDefaultMixerSelect(uint32_t)
{
   // TODO: AXSetDefaultMixerSelect
   return 0;
}

uint32_t
AXSetDRCVSMode(uint32_t)
{
   // TODO: AXSetDRCVSMode
   return 0;
}

uint32_t
AXGetAuxCallback(uint32_t, uint32_t, uint32_t, wfunc_ptr<void> *func, be_ptr<void> *userData)
{
   // TODO: AXGetAuxCallback
   if (func != nullptr) {
      *func = nullptr;
      *userData = nullptr;
   }
   return 0;
}

uint32_t AXRegisterAuxCallback(uint32_t, uint32_t, uint32_t, wfunc_ptr<void> func, void *userData)
{
   // TODO: AXRegisterAuxCallback
   return 0;
}

uint32_t AXGetDeviceFinalMixCallback(uint32_t, wfunc_ptr<void> *func)
{
   // TODO: AXGetDeviceFinalMixCallback
   *func = nullptr;
   return 0;
}

uint32_t AXRegisterDeviceFinalMixCallback(uint32_t, wfunc_ptr<void>)
{
   // TODO: AXRegisterDeviceFinalMixCallback
   return 0;
}

uint32_t AXGetDeviceMode(uint32_t, be_val<uint32_t>*)
{
   // TODO: AXGetDeviceMode
   return 0;
}

int32_t AXRmtGetSamplesLeft()
{
   // TODO: AXRmtGetSamplesLeft
   return 0;
}

int32_t AXRmtAdvancePtr(int32_t)
{
   // TODO: AXRmtAdvancePtr
   return 0;
}

uint32_t AXRegisterAppFrameCallback(wfunc_ptr<void>)
{
   // TODO: AXRegisterAppFrameCallback
   return 0;
}

void
Snd_Core::registerCoreFunctions()
{
   RegisterKernelFunction(AXInit);
   RegisterKernelFunction(AXInitProfile);
   RegisterKernelFunction(AXSetDefaultMixerSelect);
   RegisterKernelFunction(AXSetDRCVSMode);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXRmtGetSamplesLeft);
   RegisterKernelFunction(AXRmtAdvancePtr);
   RegisterKernelFunction(AXRegisterAppFrameCallback);
}
