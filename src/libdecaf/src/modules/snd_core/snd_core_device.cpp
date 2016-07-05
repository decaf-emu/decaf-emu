#include "snd_core.h"
#include "snd_core_device.h"

namespace snd_core
{

static AXDeviceFinalMixCallback
gDeviceFinalMixCallback;

AXResult
AXGetDeviceMode(AXDeviceType type, be_val<AXDeviceMode>* mode)
{
   // TODO: AXGetDeviceMode
   *mode = static_cast<AXDeviceMode>(0);
   return AXResult::Success;
}

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type, be_AXDeviceFinalMixCallback *func)
{
   if (func != nullptr) {
      *func = gDeviceFinalMixCallback;
   }

   return AXResult::Success;
}

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type, AXDeviceFinalMixCallback func)
{
   gDeviceFinalMixCallback = func;
   return AXResult::Success;
}

AXResult
AXGetAuxCallback(AXDeviceType type, uint32_t, uint32_t, be_AXAuxCallback *callback, be_ptr<void> *userData)
{
   // TODO: AXGetAuxCallback
   if (callback) {
      *callback = nullptr;
   }

   if (userData) {
      *userData = nullptr;
   }

   return AXResult::Success;
}

AXResult
AXRegisterAuxCallback(AXDeviceType type, uint32_t, uint32_t, AXAuxCallback callback, void *userData)
{
   // TODO: AXRegisterAuxCallback
   return AXResult::Success;
}

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type, uint32_t, uint32_t)
{
   // TODO: AXSetDeviceLinearUpsampler
   return AXResult::Success;
}

AXResult
AXSetDeviceCompressor(AXDeviceType type, uint32_t)
{
   // TODO: AXSetDeviceCompressor
   return AXResult::Success;
}

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type, uint32_t)
{
   // TODO: AXSetDeviceUpsampleStage
   return AXResult::Success;
}

AXResult
AXSetDeviceVolume(AXDeviceType type, uint32_t id, uint16_t volume)
{
   return AXResult::Success;
}

void
Module::registerDeviceFunctions()
{
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
   RegisterKernelFunction(AXSetDeviceLinearUpsampler);
   RegisterKernelFunction(AXSetDeviceCompressor);
   RegisterKernelFunction(AXSetDeviceUpsampleStage);
   RegisterKernelFunction(AXSetDeviceVolume);
}

} // namespace snd_core
