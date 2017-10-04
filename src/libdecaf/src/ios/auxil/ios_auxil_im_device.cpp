#include "ios_auxil_im_device.h"
#include "ios_auxil_usr_cfg_ipc.h"
#include "ios/kernel/ios_kernel_process.h"

#include "ios/ios_stackobject.h"

#include <map>

namespace ios::auxil::internal
{

struct StaticImDeviceData
{
   be2_val<UCHandle> ucHandle;
   be2_array<uint32_t, IMParameter::Max> parameters;
   be2_array<uint32_t, IMParameter::Max> nvParameters;
   be2_array<uint32_t, IMParameter::Max> defaultValues;
};

static phys_ptr<StaticImDeviceData>
sData = nullptr;

static std::map<IMParameter, const char *>
sParameterKey {
   { IMParameter::InactiveSeconds,  "inactive_seconds" },
   { IMParameter::DimEnabled,       "dim_enable" },
   { IMParameter::DimPeriod,        "dim_seconds" },
   { IMParameter::APDEnabled,       "apd_enable" },
   { IMParameter::APDPeriod,        "apd_seconds" },
   { IMParameter::ResetEnable,      "reset_enable" },
   { IMParameter::ResetSeconds,     "reset_secnds" },
   { IMParameter::PowerOffEnable,   "power_off_enable" },
   { IMParameter::ApdOccurred,      "apd_occurred" },
   { IMParameter::DimEnableTv,      "dim_tv_enable" },
   { IMParameter::DimEnableDrc,     "dim_drc_enable" },
};

Error
IMDevice::copyParameterFromNv()
{
   for (auto i = 0u; i < IMParameter::Max; ++i) {
      if (i == IMParameter::PowerOffEnable) {
         // Do not copy parameter 7??
         continue;
      }

      setParameter(static_cast<IMParameter>(i), phys_addrof(sData->nvParameters[i]));
   }

   return Error::OK;
}

Error
IMDevice::getHomeButtonParams(phys_ptr<IMGetHomeButtonParamResponse> response)
{
   response->type = IMHomeButtonType::None;
   response->index = 0;
   return Error::OK;
}

Error
IMDevice::getParameter(IMParameter parameter,
                       phys_ptr<uint32_t> value)
{
   if (parameter >= IMParameter::Max) {
      return Error::Invalid;
   }

   *value = sData->parameters[parameter];
   return Error::OK;
}

Error
IMDevice::getNvParameter(IMParameter parameter,
                         phys_ptr<uint32_t> value)
{
   if (parameter >= IMParameter::Max) {
      return Error::Invalid;
   }

   *value = sData->nvParameters[parameter];
   return Error::OK;
}

Error
IMDevice::getTimerRemaining(IMTimer timer,
                            phys_ptr<uint32_t> value)
{
   *value = 30u * 60;
   return Error::OK;
}

Error
IMDevice::setParameter(IMParameter parameter,
                       phys_ptr<uint32_t> value)
{
   if (parameter >= IMParameter::Max) {
      return Error::Invalid;
   }

   sData->parameters[parameter] = *value;
   return Error::OK;
}

Error
IMDevice::setNvParameter(IMParameter parameter,
                         phys_ptr<uint32_t> value)
{
   if (parameter >= IMParameter::Max) {
      return Error::Invalid;
   }

   sData->nvParameters[parameter] = *value;
   return Error::OK;
}

void
initialiseStaticImDeviceData()
{
   sData = phys_cast<StaticImDeviceData>(kernel::allocProcessStatic(sizeof(StaticImDeviceData)));
   sData->defaultValues[IMParameter::InactiveSeconds] = 0xAu;
   sData->defaultValues[IMParameter::DimEnabled]      = 1u;
   sData->defaultValues[IMParameter::DimPeriod]       = 300u;
   sData->defaultValues[IMParameter::APDEnabled]      = 1u;
   sData->defaultValues[IMParameter::APDPeriod]       = 3600u;
   sData->defaultValues[IMParameter::ResetEnable]     = 0u;
   sData->defaultValues[IMParameter::ResetSeconds]    = 120u;
   sData->defaultValues[IMParameter::PowerOffEnable]  = 1u;
   sData->defaultValues[IMParameter::ApdOccurred]     = 0u;
   sData->defaultValues[IMParameter::DimEnableTv]     = 1u;
   sData->defaultValues[IMParameter::DimEnableDrc]    = 1u;
}

Error
initialiseImParameters()
{
   StackArray<UCSysConfig, IMParameter::Max> sysConfig;

   auto error = UCOpen();
   if (error < Error::OK) {
      return Error::Invalid;
   }

   sData->ucHandle = static_cast<UCHandle>(error);

   for (auto i = 0u; i < IMParameter::Max; ++i) {
      auto parameter = static_cast<IMParameter>(i);
      auto &cfg = sysConfig[i];
      cfg.name = std::string { "slc:im_cfg." } + sParameterKey[parameter];
      cfg.data = virt_addr { phys_addr { phys_addrof(sData->nvParameters[i]) }.getAddress() }; // EW!!!
      cfg.dataSize = 4u;
      cfg.dataType = UCDataType::UnsignedInt;
      cfg.error = UCError::OK;
      cfg.access = 0u;
   }


   auto ucError = UCReadSysConfig(sData->ucHandle,
                                  IMParameter::Max,
                                  sysConfig);
   if (ucError < UCError::OK) {
      return Error::Invalid;
   }

   auto numErrorParams = 0u;

   for (auto i = 0u; i < IMParameter::Max; ++i) {
      if (sysConfig[i].error != UCError::OK) {
         sData->nvParameters[i] = sData->defaultValues[i];
         ++numErrorParams;
      }
   }

   if (numErrorParams > 0) {
      ucError = UCWriteSysConfig(sData->ucHandle,
                                 IMParameter::Max,
                                 sysConfig);
      if (ucError < UCError::OK) {
         error = Error::Invalid;
      }
   }

   IMDevice device;
   device.copyParameterFromNv();
   return error;
}

} // namespace ios::auxil::internal
