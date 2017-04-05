#pragma once
#include "kernel_ios_device.h"
#include "modules/coreinit/coreinit_mcp.h"

#include <cstdint>

namespace kernel
{

/**
 * \ingroup kernel_ios
 * @{
 */

using coreinit::MCPError;
using coreinit::MCPSysProdSettings;

using coreinit::MCPResponseGetTitleId;

class MCPDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/mcp";

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
   MCPError getSysProdSettings(MCPSysProdSettings *settings);
   MCPError getTitleId(MCPResponseGetTitleId *response);

private:
};

/** @} */

} // namespace kernel
