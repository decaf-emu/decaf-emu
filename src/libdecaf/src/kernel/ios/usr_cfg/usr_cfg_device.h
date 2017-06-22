#pragma once
#include "usr_cfg_enum.h"
#include "usr_cfg_request.h"
#include "usr_cfg_types.h"
#include "kernel/kernel_ios_device.h"

#include <cstdint>

namespace kernel
{

namespace ios
{

namespace usr_cfg
{

/**
 * \ingroup kernel_ios_usr_cfg
 * @{
 */

class UserConfigDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/usr_cfg";

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
   UCError
   readSysConfig(UCReadSysConfigRequest *request);

   UCError
   writeSysConfig(UCWriteSysConfigRequest *request);
};

/** @} */

} // namespace usr_cfg

} // namespace ios

} // namespace kernel
