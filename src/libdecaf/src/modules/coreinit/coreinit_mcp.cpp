#include "coreinit.h"
#include "coreinit_mcp.h"
#include <common/decaf_assert.h>
#include "decaf_config.h"
#include "kernel/kernel.h"

namespace coreinit
{

static const IOSHandle
sMCPHandle = 0x12345678;

IOSHandle
MCP_Open()
{
   return sMCPHandle;
}

void
MCP_Close(IOSHandle handle)
{
   decaf_check(handle == sMCPHandle);
}

MCPError
MCP_GetSysProdSettings(IOSHandle handle, MCPSysProdSettings *settings)
{
   decaf_check(handle == sMCPHandle);

   if (!settings) {
      return MCPError::InvalidBuffer;
   }

   std::memset(settings, 0, sizeof(MCPSysProdSettings));
   settings->gameRegion = static_cast<SCIRegion>(kernel::getGameInfo().meta.region);
   settings->platformRegion = static_cast<SCIRegion>(decaf::config::system::region);
   return MCPError::OK;
}

void
Module::registerMcpFunctions()
{
   RegisterKernelFunction(MCP_Open);
   RegisterKernelFunction(MCP_Close);
   RegisterKernelFunction(MCP_GetSysProdSettings);
}

} // namespace coreinit
