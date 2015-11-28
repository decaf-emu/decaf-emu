#include "snd_core.h"
#include "snd_core_device.h"

static AXDeviceFinalMixCallback
gDeviceFinalMixCallback;

AXResult::Result
AXGetDeviceMode(AXDeviceType::Type type, be_val<AXDeviceMode::Mode>* mode)
{
   // TODO: AXGetDeviceMode
   *mode = static_cast<AXDeviceMode::Mode>(0);
   return AXResult::Success;
}

AXResult::Result
AXGetDeviceFinalMixCallback(AXDeviceType::Type type, be_AXDeviceFinalMixCallback *func)
{
   if (func != nullptr) {
      *func = gDeviceFinalMixCallback;
   }

   return AXResult::Success;
}

AXResult::Result
AXRegisterDeviceFinalMixCallback(AXDeviceType::Type type, AXDeviceFinalMixCallback func)
{
   gDeviceFinalMixCallback = func;
   return AXResult::Success;
}

AXResult::Result
AXSetDRCVSMode(AXDRCVSMode::Mode mode)
{
   if (mode >= AXDRCVSMode::Max) {
      return AXResult::InvalidDRCVSMode;
   }

   // TODO: AXSetDRCVSMode
   return AXResult::Success;
}

AXResult::Result
AXGetAuxCallback(AXDeviceType::Type type, uint32_t, uint32_t, be_AXAuxCallback *callback, be_ptr<void> *userData)
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

AXResult::Result
AXRegisterAuxCallback(AXDeviceType::Type type, uint32_t, uint32_t, AXAuxCallback callback, void *userData)
{
   // TODO: AXRegisterAuxCallback
   return AXResult::Success;
}

void
Snd_Core::registerDeviceFunctions()
{
   RegisterKernelFunction(AXGetDeviceMode);
   RegisterKernelFunction(AXGetDeviceFinalMixCallback);
   RegisterKernelFunction(AXRegisterDeviceFinalMixCallback);
   RegisterKernelFunction(AXSetDRCVSMode);
   RegisterKernelFunction(AXGetAuxCallback);
   RegisterKernelFunction(AXRegisterAuxCallback);
}
