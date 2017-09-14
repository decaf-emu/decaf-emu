#pragma once
#include "ios/dev/ios_device.h"
#include "mcp_enum.h"
#include "mcp_request.h"
#include "mcp_response.h"
#include "mcp_types.h"

#include <cstdint>

namespace ios
{

namespace dev
{

namespace mcp
{

/**
 * \defgroup ios_dev_mcp /dev/mcp
 * \ingroup ios_dev
 * @{
 */

class MCPDevice : public IOSDevice
{
public:
   static constexpr const char *Name = "/dev/mcp";

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
   MCPError getSysProdSettings(MCPSysProdSettings *settings);
   MCPError getTitleId(MCPResponseGetTitleId *response);

private:
};

/** @} */

} // namespace mcp

} // namespace dev

} // namespace ios
