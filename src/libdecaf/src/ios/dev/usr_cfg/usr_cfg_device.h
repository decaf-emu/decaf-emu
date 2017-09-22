#pragma once
#include "ios/dev/ios_device.h"
#include "usr_cfg_enum.h"
#include "usr_cfg_request.h"
#include "usr_cfg_types.h"

#include <cstdint>

namespace ios
{

namespace dev
{

namespace usr_cfg
{

/**
 * \defgroup ios_dev_usr_cfg /dev/usr_cfg
 * \ingroup ios_dev
 * @{
 */

class UserConfigDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/usr_cfg";

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
   UCError
   readSysConfig(UCReadSysConfigRequest *request);

   UCError
   writeSysConfig(UCWriteSysConfigRequest *request);
};

/** @} */

} // namespace usr_cfg

} // namespace dev

} // namespace ios
