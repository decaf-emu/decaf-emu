#pragma once
#include "mcp_enum.h"
#include "mcp_request.h"
#include "mcp_response.h"
#include "mcp_types.h"
#include "kernel/kernel_ios_device.h"

#include <cstdint>

namespace kernel
{

namespace ios
{

namespace mcp
{

/**
 * \ingroup kernel_ios_mcp
 * @{
 */

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

} // namespace mcp

} // namespace ios

} // namespace kernel
