#pragma once
#include "im_enum.h"
#include "im_request.h"
#include "im_response.h"
#include "kernel/kernel_ios_device.h"

#include <cstdint>
#include <common/cbool.h>

namespace kernel
{

namespace ios
{

namespace im
{

/**
 * \ingroup kernel_ios_im
 * @{
 */

class IMDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/im";

public:
   virtual IOSError
   open(IOSOpenMode mode) override;

   virtual IOSError
   close() override;

   virtual IOSError
   read(void *buffer,
        size_t length) override;

   virtual IOSError
   write(void *buffer,
         size_t length) override;

   virtual IOSError
   ioctl(uint32_t request,
         void *inBuf,
         size_t inLen,
         void *outBuf,
         size_t outLen) override;

   virtual IOSError
   ioctlv(uint32_t request,
          size_t vecIn,
          size_t vecOut,
          IOSVec *vec) override;

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

} // namespace ios

} // namespace kernel
