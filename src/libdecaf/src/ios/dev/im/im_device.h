#pragma once
#include "im_enum.h"
#include "im_request.h"
#include "im_response.h"
#include "ios/dev/ios_device.h"

#include <cstdint>
#include <common/cbool.h>

namespace ios
{

namespace dev
{

namespace im
{

/**
 * \defgroup ios_dev_im /dev/im
 * \ingroup ios_dev
 * @{
 */

class IMDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/im";

public:
   virtual Error
   open(OpenMode mode) override;

   virtual Error
   close() override;

   virtual Error
   read(void *buffer,
        size_t length) override;

   virtual Error
   write(void *buffer,
         size_t length) override;

   virtual Error
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual Error
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IoctlVec *vec) override;

private:
   IMError
   getHomeButtonParams(IMGetHomeButtonParamResponse *response);

   IMError
   getParameter(IMGetParameterRequest *request,
                IMGetParameterResponse *response);

   IMError
   getNvParameter(IMGetNvParameterRequest *request,
                  IMGetNvParameterResponse *response);

   IMError
   getTimerRemaining(IMGetTimerRemainingRequest *request,
                     IMGetTimerRemainingResponse *response);

   IMError
   setParameter(IMSetParameterRequest *request);

   IMError
   setNvParameter(IMSetNvParameterRequest *request);

private:
   BOOL mAPDEnabled = FALSE;
   uint32_t mAPDPeriodSeconds = 60 * 60;
   BOOL mDimEnabled = FALSE;
   uint32_t mDimPeriodSeconds = 30 * 60;
   uint32_t mUnknownParameter5 = 0;
   BOOL mDimEnableTv = FALSE;
   BOOL mDimEnableDrc = FALSE;
};

/** @} */

} // namespace im

} // namespace dev

} // namespace ios
