#include "coreinit.h"
#include "coreinit_mcp.h"
#include "common/decaf_assert.h"
#include "decaf_config.h"

namespace coreinit
{

static const IOHandle
sMCPHandle = 0x12345678;

IOHandle
MCP_Open()
{
   return sMCPHandle;
}

void
MCP_Close(IOHandle handle)
{
   decaf_check(handle == sMCPHandle);
}

IOError
MCP_GetSysProdSettings(IOHandle handle, MCPSysProdSettings *settings)
{
   if (handle != sMCPHandle) {
      return IOError::Generic;
   }

   memset(settings, 0, sizeof(MCPSysProdSettings));
   settings->gameRegion = static_cast<SCIRegion>(decaf::config::system::region);
   settings->platformRegion = static_cast<SCIRegion>(decaf::config::system::region);
   return IOError::OK;
}

void
Module::registerMcpFunctions()
{
   RegisterKernelFunction(MCP_Open);
   RegisterKernelFunction(MCP_Close);
   RegisterKernelFunction(MCP_GetSysProdSettings);
}

} // namespace coreinit
