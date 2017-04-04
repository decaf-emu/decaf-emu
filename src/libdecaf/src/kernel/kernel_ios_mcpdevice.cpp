#include "decaf_config.h"
#include "kernel_ios_mcpdevice.h"
#include "kernel.h"

#include <cstring>

namespace kernel
{

using coreinit::MCPCommand;
using coreinit::SCIRegion;

IOSError
MCPDevice::open(IOSOpenMode mode)
{
   return IOSError::OK;
}


IOSError
MCPDevice::close()
{
   return IOSError::OK;
}


IOSError
MCPDevice::read(void *buffer,
                size_t length)
{
   return static_cast<IOSError>(MCPError::Unsupported);
}


IOSError
MCPDevice::write(void *buffer,
                 size_t length)
{
   return static_cast<IOSError>(MCPError::Unsupported);
}


IOSError
MCPDevice::ioctl(uint32_t cmd,
                 void *inBuf,
                 size_t inLen,
                 void *outBuf,
                 size_t outLen)
{
   return static_cast<IOSError>(MCPError::Unsupported);
}


IOSError
MCPDevice::ioctlv(uint32_t cmd,
                  size_t vecIn,
                  size_t vecOut,
                  IOSVec *vec)
{
   auto result = MCPError::OK;

   switch (static_cast<MCPCommand>(cmd)) {
   case MCPCommand::GetSysProdSettings:
      decaf_check(vecIn == 0);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(MCPSysProdSettings));
      result = getSysProdSettings(be_ptr<MCPSysProdSettings> { vec[0].vaddr });
      break;
   default:
      result = MCPError::Unsupported;
   }

   return static_cast<IOSError>(result);
}


MCPError
MCPDevice::getSysProdSettings(MCPSysProdSettings *settings)
{
   std::memset(settings, 0, sizeof(MCPSysProdSettings));
   settings->gameRegion = static_cast<SCIRegion>(kernel::getGameInfo().meta.region);
   settings->platformRegion = static_cast<SCIRegion>(decaf::config::system::region);
   return MCPError::OK;
}

} // namespace kernel
