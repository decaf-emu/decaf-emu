#include "coreinit.h"
#include "coreinit_mcp.h"

static const IOHandle
gMCPHandle = 0x12345678;

IOHandle
MCP_Open()
{
   return gMCPHandle;
}

void
MCP_Close(IOHandle handle)
{
   assert(handle == gMCPHandle);
}

IOError
MCP_GetSysProdSettings(IOHandle handle, MCPSysProdSettings *settings)
{
   if (handle != gMCPHandle) {
      return IOError::Generic;
   }

   memset(settings, 0, sizeof(MCPSysProdSettings));
   settings->gameRegion = SCIRegion::US;
   settings->platformRegion = SCIRegion::US;
   return IOError::OK;
}

void
CoreInit::registerMcpFunctions()
{
   RegisterKernelFunction(MCP_Open);
   RegisterKernelFunction(MCP_Close);
   RegisterKernelFunction(MCP_GetSysProdSettings);
}
