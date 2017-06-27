#include "mcp_device.h"
#include "decaf_config.h"
#include "kernel/kernel.h"

#include <cstring>

namespace ios
{

namespace dev
{

namespace mcp
{

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
   auto result = MCPError::OK;

   switch (static_cast<MCPCommand>(cmd)) {
   case MCPCommand::GetTitleId:
      decaf_check(inBuf == nullptr);
      decaf_check(inLen == 0);
      result = getTitleId(reinterpret_cast<MCPResponseGetTitleId *>(outBuf));
      break;
   default:
      result = MCPError::Unsupported;
   }

   return static_cast<IOSError>(result);
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
   {
      decaf_check(vecIn == 0);
      decaf_check(vecOut == 1);
      decaf_check(vec[0].len == sizeof(MCPSysProdSettings));
      auto settings = cpu::PhysicalPointer<MCPSysProdSettings> { vec[0].paddr };
      result = getSysProdSettings(settings.getRawPointer());
      break;
   }
   default:
      result = MCPError::Unsupported;
   }

   return static_cast<IOSError>(result);
}


MCPError
MCPDevice::getSysProdSettings(MCPSysProdSettings *settings)
{
   std::memset(settings, 0, sizeof(MCPSysProdSettings));
   settings->gameRegion = static_cast<MCPRegion>(kernel::getGameInfo().meta.region);
   settings->platformRegion = static_cast<MCPRegion>(decaf::config::system::region);
   return MCPError::OK;
}


MCPError
MCPDevice::getTitleId(MCPResponseGetTitleId *response)
{
   response->titleId = kernel::getGameInfo().meta.title_id;
   return MCPError::OK;
}

} // namespace mcp

} // namespace dev

} // namespace ios
