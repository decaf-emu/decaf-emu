#pragma once
#include "ios_auxil_enum.h"
#include "ios_auxil_im_request.h"
#include "ios_auxil_im_response.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"

namespace ios::auxil::internal
{

class IMDevice
{
public:
   Error
   copyParameterFromNv();

   Error
   getHomeButtonParams(phys_ptr<IMGetHomeButtonParamResponse> response);

   Error
   getParameter(IMParameter parameter,
                phys_ptr<uint32_t> value);

   Error
   getNvParameter(IMParameter parameter,
                  phys_ptr<uint32_t> value);

   Error
   getTimerRemaining(IMTimer timer,
                     phys_ptr<uint32_t> value);

   Error
   setParameter(IMParameter parameter,
                phys_ptr<uint32_t> value);

   Error
   setNvParameter(IMParameter parameter,
                  phys_ptr<uint32_t> value);

private:
};

void
initialiseStaticImDeviceData();

Error
initialiseImParameters();

} // namespace ios::auxil::internal
