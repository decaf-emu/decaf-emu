#include "ios_auxil_im_device.h"

namespace ios::auxil::internal
{

struct ImDeviceData
{
   be2_array<uint32_t, IMParameter::Max> parameters;
   be2_array<uint32_t, IMParameter::Max> nvParameters;
};

static phys_ptr<ImDeviceData>
sData;

Error
IMDevice::copyParameterFromNv()
{
   for (auto i = 0u; i < IMParameter::Max; ++i) {
      if (i == IMParameter::Unknown7) {
         // Do not copy parameter 7
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

} // namespace ios::auxil::internal
